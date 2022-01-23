#pragma once

#include "vk.h"

struct RenderImpl;
struct SwapChain {
    struct RenderImpl* r;
    VkSwapchainKHR swapchain;
    VkFormat im_format;
    VkExtent2D extent;
    VkImage* images;
    int n_images;
    VkImage depth_image;
    VkDeviceMemory depth_image_memory;
    VkFormat depth_format;
};

void sc_init(struct SwapChain* sc, struct RenderImpl* r);
void sc_destroy(struct SwapChain* sc);
