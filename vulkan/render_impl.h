#pragma once

#include "vk.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <render/render.h>

#include "swapchain.h"
#include "renderpass.h"
#include "rendertarget.h"
#include "commandbuffer.h"
#include "device.h"
#include "frame.h"

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
    struct Device device;
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
    struct Frame frames[5];
    struct Frame* frame;

    //
    uint32_t image_index;
    VkCommandBuffer buffer;

    VkSemaphore recycled_semaphores[10];
    int n_recycled_semaphores;

    //
    VkViewport viewport;
    VkRect2D scissor;
    int update_viewport;

    struct VkStats* stats;
};
