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

void cb_init(struct CommandBuffer* d, struct RenderImpl* r);
void cb_destroy(struct CommandBuffer* d);
void cb_begin(struct CommandBuffer* d, VkCommandBuffer buffer);
void cb_end(struct CommandBuffer* d, VkCommandBuffer buffer);
VkCommandBuffer cb_acquire(struct CommandBuffer* d);
void cb_reset(struct CommandBuffer* d);
