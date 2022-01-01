#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include "swapchain.h"
#include "renderpass.h"
#include "rendertarget.h"
#include "drawcommandbuffer.h"

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

    //

    struct SwapChain sc;
    struct RenderPass rp;
    struct RenderTarget rt;
    struct DrawCommandBuffer dcb;

    //
    uint32_t image_index;
    VkCommandBuffer buffer;
};
