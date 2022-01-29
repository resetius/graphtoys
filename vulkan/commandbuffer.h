#pragma once

#include "vk.h"

struct RenderImpl;
struct CommandBuffer {
    struct RenderImpl* r;
    VkDevice dev;
    VkCommandPool pool;
    VkCommandBuffer* buffers;
    int n_buffers;
};

void cb_init(struct DrawCommandBuffer* d, struct RenderImpl* r);
void cb_destroy(struct DrawCommandBuffer* d);
void cb_begin(struct DrawCommandBuffer* d, VkCommandBuffer buffer);
void cb_end(struct DrawCommandBuffer* d, VkCommandBuffer buffer);
