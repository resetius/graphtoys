#pragma once

#include <vulkan/vulkan.h>

struct RenderImpl;
struct DrawCommandBuffer {
    struct RenderImpl* r;
    VkDevice dev;
    VkCommandPool pool;
    VkCommandBuffer* buffers;
    int n_buffers;
};

void dcb_init(struct DrawCommandBuffer* d, struct RenderImpl* r);
void dcb_destroy(struct DrawCommandBuffer* d);
