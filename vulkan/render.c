#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include "renderpass.h"
#include "swapchain.h"
#include "render_impl.h"

static void free_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    dcb_destroy(&r->dcb);
    rt_destroy(&r->rt);
    rp_destroy(&r->rp);
    sc_destroy(&r->sc);
    free(r->modes);
    free(r->formats);
    free(r);
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
    }

    vkAcquireNextImageKHR(
        r->log_dev,
        r->sc.swapchain,
        (uint64_t)-1,
        r->imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &r->image_index);

    vkWaitForFences(r->log_dev, 1, &r->infl_fences[r->image_index], VK_TRUE, (uint64_t)-1);
    vkResetFences(r->log_dev, 1, &r->infl_fences[r->image_index]);

    r->buffer = r->dcb.buffers[r->image_index];

    dcb_begin(&r->dcb, r->buffer);

    if (r->update_viewport) {
        VkRect2D scissor = {{0, 0}, r->sc.extent};
        r->scissor = scissor;
        r->update_viewport = 0;

        vkCmdSetViewport(r->buffer, 0, 1, &r->viewport);
        vkCmdSetScissor(r->buffer, 0, 1, &r->scissor);
    }

    VkClearColorValue color =  {
        .float32 = {0.0f, 0.0f, 0.0f, 1.0f}
    };
    VkClearValue clear_color = {
        .color = color
    };
    VkClearValue clear_depth = {
        .depthStencil = {1.0f, 0}
    };
    VkClearValue values[] = { clear_color, clear_depth };

    rp_begin(
        &r->rp,
        values, 2,
        r->buffer,
        r->rt.framebuffers[r->image_index],
        r->sc.extent);
}

static void draw_end_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;

    rp_end(&r->rp, r->buffer);
    dcb_end(&r->dcb, r->buffer);

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &r->buffer,
        .pWaitDstStageMask = waitStages,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &r->imageAvailableSemaphore,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &r->renderFinishedSemaphore
    };

    vkQueueSubmit(r->g_queue, 1, &submitInfo, r->infl_fences[r->image_index]);

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &r->renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &r->sc.swapchain,
        .pImageIndices = &r->image_index
    };

    vkQueuePresentKHR(r->p_queue, &presentInfo);
    vkQueueWaitIdle(r->p_queue);
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

    r->graphics_family = r->present_family = -1;

    for (i = 0; i < count; i++) {
        VkBool32 flag = 0;
        if (families[i].queueCount > 0 && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            r->graphics_family = i;
        }

        vkGetPhysicalDeviceSurfaceSupportKHR(r->phy_dev, i, r->surface, &flag);

        if (families[i].queueCount > 0 && flag) {
            r->present_family = i;
        }

        if (r->present_family >= 0 && r->graphics_family >= 0) {
            break;
        }
    }
    printf("Present: %d, graphics: %d\n",
           r->present_family,
           r->graphics_family);
    free(families);
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    uint32_t deviceCount = 0;
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset"
    };
    uint32_t nDeviceExtensions = 1;
    VkPhysicalDevice devices[100];
    VkDeviceQueueCreateInfo queueCreateInfo[2];
    uint32_t nQueueCreateInfo = 2;
    float queuePriority = 1.0f;
    int i;
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
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        printf("Name: '%s'\n", props.deviceName);
        // TODO: check device
        r->phy_dev = devices[i];
        break;
    }

    find_queue_families(r);
    printf("graphics_family: %d, present_family: %d\n",
           r->graphics_family,
           r->present_family);

    if (r->graphics_family == r->present_family) {
        nQueueCreateInfo = 1;
        queueCreateInfo[0].queueFamilyIndex = r->graphics_family;
    } else {
        queueCreateInfo[0].queueFamilyIndex = r->graphics_family;
        queueCreateInfo[1].queueFamilyIndex = r->present_family;
    }

    for (i = 0; i < nQueueCreateInfo; i++) {
        VkDeviceQueueCreateInfo info = {
            .queueFamilyIndex = queueCreateInfo[i].queueFamilyIndex,
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
        queueCreateInfo[i] = info;
    }

    uint32_t allDevExts = 0;
    vkEnumerateDeviceExtensionProperties(r->phy_dev, NULL, &allDevExts, NULL);
    VkExtensionProperties* exts = malloc(allDevExts*sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(r->phy_dev, NULL, &allDevExts, exts);
    printf("Supported device extensions:\n");
    for (int i = 0; i < allDevExts; i++) {
        if (!strcmp(exts[i].extensionName, "VK_KHR_portability_subset")) {
            nDeviceExtensions = 2; // enable it
        }
        printf("'%s'\n", exts[i].extensionName);
    }
    printf("\n");
    free(exts);

    VkPhysicalDeviceFeatures deviceFeatures = {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfo,
        .queueCreateInfoCount = nQueueCreateInfo,
        .pEnabledFeatures = &deviceFeatures, //
        .enabledExtensionCount = nDeviceExtensions,
        .ppEnabledExtensionNames = deviceExtensions,
        .enabledLayerCount = 0
    };

    if (vkCreateDevice(r->phy_dev, &createInfo, NULL, &r->log_dev) != VK_SUCCESS) {
        fprintf(stderr, "Cannot create logical device\n");
        exit(-1);
    }

    vkGetDeviceQueue(r->log_dev, r->graphics_family, 0, &r->g_queue);
    vkGetDeviceQueue(r->log_dev, r->present_family, 0, &r->p_queue);

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
    dcb_init(&r->dcb, r);
    printf("Drawcommandbuffer initialized\n");

    r->n_infl_fences = sizeof(r->infl_fences)/sizeof(VkFence);
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

	VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

	vkCreateSemaphore(r->log_dev, &semaphoreInfo, NULL, &r->imageAvailableSemaphore);
	vkCreateSemaphore(r->log_dev, &semaphoreInfo, NULL, &r->renderFinishedSemaphore);

	for (int i = 0; i < r->n_infl_fences; i++) {
		if (vkCreateFence(r->log_dev, &fenceCreateInfo, NULL, &r->infl_fences[i]) != VK_SUCCESS) {

            fprintf(stderr, "Failed to create synchronization objects per frame\n");
            exit(-1);
        }
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

struct PipelineBuilder* pipeline_builder_vulkan(struct Render* r);
struct Char* rend_vulkan_char_new(struct Render* r1, wchar_t ch, void* bm);

struct Render* rend_vulkan_new() {
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
    };
    uint32_t extensionCount = 0;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    uint32_t allExtCount = 0;
    VkExtensionProperties* exts;
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
    extensionNames[i++] = "VK_KHR_get_physical_device_properties2"; // mac?
    //extensionNames[i++] = "VK_KHR_Maintenance1"; // flip viewport, see https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
    extensionCount = i;
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Graph Toys",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "GraphToys",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 2, 0)
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

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (vkCreateInstance(&vkInstanceInfo, NULL, &r->instance) != VK_SUCCESS) {
        fprintf(stderr, "Cannot initialize vulkan\n");
        exit(-1);
    }
    free(extensionNames);

    return (struct Render*)r;
}
