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
    VkPhysicalDevice phy_dev;
    GLFWwindow* window;
};

static void free_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    free(r);
}

static void set_window_(struct Render* r1, void* w) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    r->window = w;
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    uint32_t deviceCount = 0;
    VkPhysicalDevice devices[100];
    int i;
    if (glfwCreateWindowSurface(r->instance, r->window, NULL, &r->surface) != VK_SUCCESS) {
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
