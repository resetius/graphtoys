#pragma once

#include "vk.h"

struct RenderImpl;
struct CommandBuffer {
    VkDevice dev;
    VkCommandPool pool;
};

// Buffers from the pool may only be submitted to queues of `family`.
void cb_init(struct CommandBuffer* d, struct RenderImpl* r, uint32_t family);
void cb_destroy(struct CommandBuffer* d);
void cb_begin(struct CommandBuffer* d, VkCommandBuffer buffer);
void cb_end(struct CommandBuffer* d, VkCommandBuffer buffer);
VkCommandBuffer cb_acquire(struct CommandBuffer* d);
void cb_reset(struct CommandBuffer* d);
