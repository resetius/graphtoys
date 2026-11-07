#pragma once

#include "vk.h"
#include "commandbuffer.h"

struct RenderImpl;

struct Frame {
    struct CommandBuffer cb;
    VkCommandBuffer cmd;
    struct CommandBuffer compute_cb;
    VkCommandBuffer compute_cmd;
    VkFence fence;
    VkSemaphore acquire_image;
    VkSemaphore render_finished;
    // Signaled by this frame's compute submit, waited by its graphics submit.
    // The frame fence makes it unsignaled again before the frame is reused.
    VkSemaphore compute_semaphore;
};

void frame_init(struct Frame* frame, struct RenderImpl* r);
void frame_destroy(struct Frame* frame);

VkCommandBuffer frame_cb(struct Frame* frame);
VkCommandBuffer frame_compute_cb(struct Frame* frame);
