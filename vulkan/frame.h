#pragma once

#include "vk.h"
#include "commandbuffer.h"

struct RenderImpl;

struct Frame {
    struct CommandBuffer cb;
    VkCommandBuffer cmd;
    VkFence fence;
    VkSemaphore acquire_image;
    VkSemaphore render_finished;
};

void frame_init(struct Frame* frame, struct RenderImpl* r);
void frame_destroy(struct Frame* frame);

VkCommandBuffer frame_cb(struct Frame* frame);
