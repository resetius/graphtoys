#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vk.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>
#include <lib/config.h>

#include "device.h"
#include "renderpass.h"
#include "swapchain.h"
#include "render_impl.h"
#include "stats.h"

struct BufferManager* buf_mgr_vulkan_new(struct Render* r);
void vk_load_global();
void vk_load_instance(VkInstance instance);
void vk_load_device(VkDevice device);
struct Texture* vk_tex_new(struct Render* r, void* data, enum TexType tex_type);

static void free_(struct Render* r1) {
    int i;
    struct RenderImpl* r = (struct RenderImpl*)r1;
    rt_destroy(&r->rt);
    rp_destroy(&r->rp);
    sc_destroy(&r->sc);
    cb_destroy(&r->compute_cmd);
    vk_stats_free(r->stats);

    for (i = 0; i < r->sc.n_images; i++) {
        frame_destroy(&r->frames[i]);
    }

    vkDestroySemaphore(r->log_dev, r->compute_semaphore, NULL);

    for (i = 0; i < r->n_recycled_semaphores; i++) {
        vkDestroySemaphore(r->log_dev, r->recycled_semaphores[i], NULL);
    }

    vkDestroyQueryPool(r->log_dev, r->timestamps, NULL);

    vkDestroyDevice(r->log_dev, NULL);
    vkDestroySurfaceKHR(r->instance, r->surface, NULL);
    vkDestroyInstance(r->instance, NULL);

    free(r->modes);
    free(r->formats);
    free(r);
}

static void compute_counters_begin_(struct RenderImpl* r)
{
    r->timestamp = r->query*r->queries_per_frame;

    vkCmdResetQueryPool(
        r->compute_buffer, r->timestamps,
        r->query*r->queries_per_frame, r->queries_per_frame);

    vkCmdWriteTimestamp(
        r->compute_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        r->timestamps,
        r->timestamp++
        );
}

static void compute_counters_end_(struct RenderImpl* r)
{
    int prev_query = (r->query+1) % r->queries_delay;
    uint64_t data[4*256];
    VkResult result = vkGetQueryPoolResults(
        r->log_dev, r->timestamps,
        prev_query*r->queries_per_frame,
        r->timestamp-r->query*r->queries_per_frame,
        sizeof(data), data, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT);

    if (result == VK_SUCCESS) {
        uint64_t s = data[0];
        // i = 0 -- technical counter
        for (int i = 1; i < r->timestamp-r->query*r->queries_per_frame&&i<32;i++) {
            int j = r->query2counter[i];
            r->counters[j].value += data[i] - s;
            r->counters[j].count ++;
            s = data[i];
        }
    }

    r->query = (r->query+1)%r->queries_delay;
}

static void print_compute_stats_(struct Render* r1)
{
    struct RenderImpl* r = (struct RenderImpl*)r1;
    double total = 0;
    for (int i = 0; i < 32; i++) {
        if (r->counters[i].count) {
            double value = r->counters[i].value
                *r->properties.limits.timestampPeriod*1e-6
                /r->counters[i].count;

            total += value;

            printf("%02d %s %.2fms\n", i, r->counters[i].name, value);

            r->counters[i].count = 0;
            r->counters[i].value = 0;
        }
    }
    if (total > 0) {
        printf("Total: %.2fms\n", total);
    }
}

static int counter_new_(struct Render*r1, const char* name, enum CounterType type)
{
    struct RenderImpl* r = (struct RenderImpl*)r1;
    r->counters[r->ncounters].name = name;
    r->counters[r->ncounters].type = type;
    return r->ncounters++;
}

static void counter_submit_(struct Render* r1, int id)
{
    struct RenderImpl* r = (struct RenderImpl*)r1;

    r->query2counter[r->timestamp-r->query*r->queries_per_frame] = id;

    if (r->counters[id].type == COUNTER_COMPUTE) {
        vkCmdWriteTimestamp(
            r->compute_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            r->timestamps,
            r->timestamp++
            );
    } else {
        vkCmdWriteTimestamp(
            r->buffer,
            r->counters[id].type == COUNTER_VERTEX
            ?VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
            :VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            r->timestamps,
            r->timestamp++
            );
    }
}

static void draw_begin_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;

    if (r->update_viewport) {
        vkDeviceWaitIdle(r->log_dev);
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->phy_dev, r->surface, &r->caps);

        rt_destroy(&r->rt);
        rp_destroy(&r->rp);
        sc_destroy(&r->sc);

        sc_init(&r->sc, r);
        rp_init(&r->rp, r->log_dev, r->sc.im_format, r->sc.depth_format);
        rt_init(&r->rt, r);
        r->update_viewport = 0;
        VkRect2D scissor = {{0, 0}, r->sc.extent};
        r->scissor = scissor;
    }

    assert(r->n_recycled_semaphores > 0);
    VkSemaphore acquire_sem = r->recycled_semaphores[--r->n_recycled_semaphores];

    vkAcquireNextImageKHR(
        r->log_dev,
        r->sc.swapchain,
        UINT64_MAX,
        acquire_sem,
        VK_NULL_HANDLE,
        &r->image_index);

    r->frame = &r->frames[r->image_index];
    if (r->frame->acquire_image != VK_NULL_HANDLE) {
        r->recycled_semaphores[r->n_recycled_semaphores++] = r->frame->acquire_image;
        r->frame->acquire_image = VK_NULL_HANDLE;
    }
    r->frame->acquire_image = acquire_sem;

    vkWaitForFences(r->log_dev, 1, &r->frame->fence, VK_TRUE, UINT64_MAX);
    vkResetFences(r->log_dev, 1, &r->frame->fence);

    r->buffer = frame_cb(r->frame);
    cb_begin(&r->frame->cb, r->buffer);

    r->compute_buffer = cb_acquire(&r->compute_cmd);
    cb_begin(&r->compute_cmd, r->compute_buffer);

    vkCmdSetViewport(r->buffer, 0, 1, &r->viewport);
    vkCmdSetScissor(r->buffer, 0, 1, &r->scissor);

    VkClearValue clear_depth = {
        .depthStencil = {1.0f, 0}
    };
    VkClearValue values[] = { r->clear_color, clear_depth };

    rp_begin(
        &r->rp,
        values, 2,
        r->buffer,
        r->rt.framebuffers[r->image_index],
        r->sc.extent);

    compute_counters_begin_(r);
}

static void draw_end_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;

    rp_end(&r->rp, r->buffer);
    cb_end(&r->frame->cb, r->buffer);
    cb_end(&r->compute_cmd, r->compute_buffer);

    VkPipelineStageFlags computeWaitStages[] = {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

    VkSubmitInfo computeSubmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &r->compute_buffer,
        .pWaitDstStageMask = computeWaitStages,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &r->frame->acquire_image,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &r->compute_semaphore,
    };
    vkQueueSubmit(r->c_queue, 1, &computeSubmitInfo, VK_NULL_HANDLE);
    //cb_reset(&r->compute_cmd);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore waitSemaphores[] = {
        //r->frame->acquire_image,
        r->compute_semaphore
    };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &r->buffer,
        .pWaitDstStageMask = waitStages,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &r->frame->render_finished
    };

    vkQueueSubmit(r->g_queue, 1, &submitInfo, r->frame->fence);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &r->frame->render_finished,
        .swapchainCount = 1,
        .pSwapchains = &r->sc.swapchain,
        .pImageIndices = &r->image_index
    };

    vkQueuePresentKHR(r->p_queue, &presentInfo);

    compute_counters_end_(r);
}

static void set_window_(struct Render* r1, void* w) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    r->window = w;
}

static void find_queue_families(struct RenderImpl* r) {
    uint32_t count = 0;
    VkQueueFamilyProperties* families;
    int i;
    vkGetPhysicalDeviceQueueFamilyProperties(r->phy_dev, &count, NULL);

    printf("Found %d queue families\n", count);
    families = malloc(count*sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(r->phy_dev, &count, families);

    r->compute_family = r->graphics_family = r->present_family = -1;

    for (i = 0; i < count; i++) {
        VkBool32 flag = 0;
        if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            r->graphics_family = i;
        }

        if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            r->compute_family = i;
        }

        vkGetPhysicalDeviceSurfaceSupportKHR(r->phy_dev, i, r->surface, &flag);

        if (families[i].queueCount > 0 && flag) {
            r->present_family = i;
        }

        if (r->present_family >= 0 && r->graphics_family >= 0 && r->compute_family >= 0) {
            break;
        }
    }
    printf("Present: %d, graphics: %d, compute: %d\n",
           r->present_family,
           r->graphics_family,
           r->compute_family);
    free(families);
}

static int cmp_int(const void* a, const void* b) {
    return *(const int*)a-*(const int*)b;
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    uint32_t deviceCount = 0;
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
        "VK_KHR_performance_query",
    };
    uint32_t nDeviceExtensions = sizeof(deviceExtensions)/sizeof(const char*);
    VkPhysicalDevice devices[100];
    VkDeviceQueueCreateInfo queueCreateInfo[2];
    uint32_t nQueueCreateInfo = 2;
    float queuePriority = 1.0f;
    int i,j;
    if (glfwCreateWindowSurface(r->instance, r->window, NULL, &r->surface) != VK_SUCCESS)
    {
        fprintf(stderr, "Cannot create window surface\n");
        exit(-1);
    }
    printf("Vulkan surface created\n");

    vkEnumeratePhysicalDevices(r->instance, &deviceCount, NULL);
    printf("Found devices: %d\n", deviceCount);
    if (deviceCount == 0) { exit(-1); }
    if (deviceCount > sizeof(devices)/sizeof(VkPhysicalDevice)) {
        printf("Cut devices\n");
    }

    vkEnumeratePhysicalDevices(r->instance, &deviceCount, devices);

    printf("Devices:\n");
    for (i = 0; i < deviceCount; i++) {
        vkGetPhysicalDeviceProperties(devices[i], &r->properties);
        printf("Name: %d: '%s'\n", i, r->properties.deviceName);
        // TODO: check device
    }
    printf("Timestamp period: %f\n", r->properties.limits.timestampPeriod);

    int dev_id = cfg_geti_def(r->cfg.cfg, "dev", 0);
    if (dev_id < deviceCount) {
        printf("Using device: %d\n", dev_id);
        r->phy_dev = devices[dev_id];
    }

    vkGetPhysicalDeviceFeatures(r->phy_dev, &r->features);
	vkGetPhysicalDeviceMemoryProperties(r->phy_dev, &r->memory_properties);

    find_queue_families(r);
    printf("graphics_family: %d, present_family: %d, compute_family: %d\n",
           r->graphics_family,
           r->present_family,
           r->compute_family);

    int families[3] = {r->graphics_family, r->present_family, r->compute_family};
    qsort(families, 3, sizeof(int), cmp_int);

    j = 0;
    for (i = 0; i < 3; i++) {
        if (j == 0 || families[i] != families[j-1]) {
            families[j++] = families[i];
        }
    }
    nQueueCreateInfo = j;
    for (i = 0; i < j; i++) {
        queueCreateInfo[i].queueFamilyIndex = families[i];
    }

    // TODO: hide into Device
    for (i = 0; i < nQueueCreateInfo; i++) {
        VkDeviceQueueCreateInfo info = {
            .queueFamilyIndex = queueCreateInfo[i].queueFamilyIndex,
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
        queueCreateInfo[i] = info;
    }

    device_new(&r->device, r->phy_dev, deviceExtensions, nDeviceExtensions, queueCreateInfo, nQueueCreateInfo);
    r->log_dev = r->device.dev; // TODO

    vk_load_device(r->log_dev);

    vkGetDeviceQueue(r->log_dev, r->graphics_family, 0, &r->g_queue);
    vkGetDeviceQueue(r->log_dev, r->present_family, 0, &r->p_queue);
    vkGetDeviceQueue(r->log_dev, r->compute_family, 0, &r->c_queue);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->phy_dev, r->surface, &r->caps);

    vkGetPhysicalDeviceSurfaceFormatsKHR(r->phy_dev, r->surface, &r->n_formats, NULL);
    if (r->n_formats) {
        r->formats = malloc(r->n_formats*sizeof(VkSurfaceFormatKHR));
        vkGetPhysicalDeviceSurfaceFormatsKHR(r->phy_dev, r->surface, &r->n_formats, r->formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(r->phy_dev, r->surface, &r->n_modes, NULL);
    if (r->n_modes) {
        r->modes = malloc(r->n_modes*sizeof(VkPresentModeKHR));
        vkGetPhysicalDeviceSurfacePresentModesKHR(r->phy_dev, r->surface, &r->n_modes, r->modes);
    }
    printf("formats: %d, modes: %d\n", r->n_formats, r->n_modes);

    printf("Logical device created\n");

    sc_init(&r->sc, r);
    printf("Swapchain initialized\n");
    rp_init(&r->rp, r->log_dev, r->sc.im_format, r->sc.depth_format);
    printf("Renderpass initialized\n");
    rt_init(&r->rt, r);
    printf("Rendertarget initialized\n");

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    // compute {
    cb_init(&r->compute_cmd, r);
    vkCreateSemaphore(r->log_dev, &semaphoreInfo, NULL, &r->compute_semaphore);
    // compute }

    r->n_recycled_semaphores = sizeof(r->recycled_semaphores)/sizeof(VkSemaphore);
    for (int i = 0; i < r->n_recycled_semaphores; i++) {
    	vkCreateSemaphore(r->log_dev, &semaphoreInfo, NULL, &r->recycled_semaphores[i]);
    }
    for (int i = 0; i < r->sc.n_images; i++) {
        frame_init(&r->frames[i], r);
    }

    int w = r->sc.extent.width;
    int h = r->sc.extent.height;
    VkViewport viewport = {
        .x = 0,
        .y = h,
        .width = w,
        .height = -h,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    r->viewport = viewport;
    VkRect2D scissor = {{0, 0}, r->sc.extent};
    r->scissor = scissor;

    r->stats = vk_stats_new(r);

    VkQueryPoolCreateInfo queryPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = 256,
    };
    vkCreateQueryPool(r->log_dev, &queryPoolInfo, NULL, &r->timestamps);
    r->timestamp = 0;
    r->queries_per_frame = 32;
    r->queries_delay = 3;
    r->query = 0;

    VkClearColorValue color =  {
        .float32 = {
            r->cfg.clear_color[0],
            r->cfg.clear_color[1],
            r->cfg.clear_color[2],
            r->cfg.clear_color[3]
        }
    };
    VkClearValue clear_color = {
        .color = color
    };

    memcpy(&r->clear_color, &clear_color, sizeof(clear_color));
}

static void set_viewport_(struct Render* r1, int w, int h) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    VkViewport viewport = {
        .x = 0,
        .y = h,
        .width = w,
        .height = -h,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    r->viewport = viewport;
    r->update_viewport = 1;
}

static void screenshot_(struct Render* r, void** data, int* w, int* h) {
    /* unimplemented */
    *data = NULL;
    *w = 0;
    *h = 0;
}

struct PipelineBuilder* pipeline_builder_vulkan(struct Render* r);
struct Char* rend_vulkan_char_new(struct Render* r1, wchar_t ch, void* bm);

struct Render* rend_vulkan_new(struct RenderConfig cfg) {
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = free_,
        .init = init_,
        .set_view_entity = set_window_,
        .draw_begin = draw_begin_,
        .draw_end = draw_end_,
        .set_viewport = set_viewport_,
        .pipeline = pipeline_builder_vulkan,
        .char_new =  rend_vulkan_char_new,
        .buffer_manager = buf_mgr_vulkan_new,
        .tex_new = vk_tex_new,
        .print_compute_stats = print_compute_stats_,
        .counter_new = counter_new_,
        .counter_submit = counter_submit_,
        .screenshot = screenshot_,
    };
    uint32_t extensionCount = 0;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    uint32_t allExtCount = 0;
    VkExtensionProperties* exts;

    vk_load_global();

    vkEnumerateInstanceExtensionProperties(NULL, &allExtCount, NULL);
    exts = malloc(allExtCount*sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &allExtCount, exts);
    printf("Supported instance extensions:\n");
    for (int i = 0; i < allExtCount; i++) {
        printf("'%s'\n", exts[i].extensionName);
    }
    printf("\n");
    free(exts);
    const char** extensionNames = malloc((extensionCount+10)*sizeof(char*));
    int i;
    for (i = 0; i < extensionCount; i++) {
        extensionNames[i] = glfwExtensionNames[i];
    }
    extensionNames[i++] = "VK_KHR_get_physical_device_properties2";
    extensionCount = i;
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Graph Toys",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "GraphToys",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 3, 0)
    };

    VkInstanceCreateInfo vkInstanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensionNames
    };

    printf("Loading extensions:\n");
    for (int i = 0; i < extensionCount; i++) {
        printf("'%s'\n", extensionNames[i]);
    }

    r->base = base;
    r->cfg = cfg;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (vkCreateInstance(&vkInstanceInfo, NULL, &r->instance) != VK_SUCCESS) {
        fprintf(stderr, "Cannot initialize vulkan\n");
        exit(-1);
    }
    free(extensionNames);

    vk_load_instance(r->instance);

    return (struct Render*)r;
}
