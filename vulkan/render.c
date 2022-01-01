#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

struct RenderImpl {
    struct Render base;
    VkInstance instance;
    VkSurfaceKHR surface;
    // Physical Device
    VkPhysicalDevice phy_dev;
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR* formats;
    uint32_t n_formats;
    VkPresentModeKHR* modes;
    uint32_t n_modes;
    int graphics_family;
    int present_family;
    // Logical Device
    VkDevice log_dev;
    VkQueue g_queue;
    VkQueue p_queue;
    // GLFW specific
    GLFWwindow* window;
};

static void free_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    free(r->modes);
    free(r->formats);
    free(r);
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
    free(families);
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    uint32_t deviceCount = 0;
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
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

    VkPhysicalDeviceFeatures deviceFeatures = {
        .samplerAnisotropy = VK_TRUE
    };

    VkDeviceCreateInfo createInfo = {
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
}

struct Render* rend_vulkan_new() {
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = free_,
        .init = init_,
        .set_view_entity = set_window_
    };
    uint32_t extensionCount = 0;
    const char** extensionNames = glfwGetRequiredInstanceExtensions(&extensionCount);

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Graph Toys",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "GraphToys",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_MAKE_VERSION(1, 0, 0)
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

    return (struct Render*)r;
}
