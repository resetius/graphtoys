#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

struct RenderImpl {
    struct Render base;
    VkInstance instance;
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
    //struct RenderImpl* r = (struct RenderImpl*)r1;
    fprintf(stderr, "Unimplemented\n");
    exit(-1);
}

struct Render* rend_vulkan_new() {
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = free_,
        .init = init_,
        .set_view_entity = set_window_
    };

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
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL
    };

    r->base = base;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (vkCreateInstance(&vkInstanceInfo, NULL, &r->instance) != 0) {
        fprintf(stderr, "Cannot initialize vulkan\n");
        exit(-1);
    }

    return (struct Render*)r;
}
