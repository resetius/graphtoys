#pragma once

#include <vulkan/vulkan.h>

struct RenderPass {
    VkRenderPass rp;
    VkDevice dev;
};

void rp_init(struct RenderPass* r, VkDevice logDev, VkFormat swapChainImageFormat);
void rp_destroy(struct RenderPass* r);

void rp_begin(
    struct RenderPass* r, VkClearValue* values, int n_values, VkCommandBuffer commands,
    VkFramebuffer framebuffer, VkExtent2D extent);
void rp_end(struct RenderPass* r, VkCommandBuffer commands);
