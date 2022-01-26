#pragma once

#include "vk.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include "swapchain.h"
#include "renderpass.h"
#include "rendertarget.h"
#include "drawcommandbuffer.h"

struct Texture {
    VkImage tex;
    VkDeviceMemory memory;
    VkImageView view;

    // TODO: remove it from here
    VkDescriptorSet set;
    int has_set;
};

struct RenderImpl {
    struct Render base;
    struct RenderConfig cfg;
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
    int compute_family;
    // Logical Device
    VkDevice log_dev;
    VkQueue g_queue;
    VkQueue p_queue;
    // GLFW specific
    GLFWwindow* window;

    // gpu
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties properties;
    //

    struct SwapChain sc;
    struct RenderPass rp;
    struct RenderTarget rt;
    struct DrawCommandBuffer dcb;

    //
    uint32_t image_index;
    uint32_t current_frame;
    VkCommandBuffer buffer;

    //
    VkFence infl_fences[10];
    VkFence images_infl[10];
    int n_infl_fences;

    VkSemaphore imageAvailableSemaphore[10];
    VkSemaphore renderFinishedSemaphore[10];

    //
    VkViewport viewport;
    VkRect2D scissor;
    int update_viewport;
};
