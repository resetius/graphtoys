#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "frame.h"
#include "render_impl.h"

void frame_init(struct Frame* frame, struct RenderImpl* r) {
    memset(frame, 0, sizeof(*frame));
    cb_init(&frame->cb, r, r->graphics_family);
    frame->cmd = cb_acquire(&frame->cb);
    cb_init(&frame->compute_cb, r, r->compute_family);
    frame->compute_cmd = cb_acquire(&frame->compute_cb);
    VkFenceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (vkCreateFence(frame->cb.dev, &info, NULL, &frame->fence) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create synchronization objects per frame\n");
        exit(-1);
    }

    VkSemaphoreCreateInfo sem_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    vkCreateSemaphore(frame->cb.dev, &sem_info, NULL, &frame->render_finished);
}

void frame_destroy(struct Frame* frame) {
    vkDestroyFence(frame->cb.dev, frame->fence, NULL);
    vkDestroySemaphore(frame->cb.dev, frame->render_finished, NULL);
    if (frame->acquire_image != VK_NULL_HANDLE) {
        vkDestroySemaphore(frame->cb.dev, frame->acquire_image, NULL);
    }
    cb_destroy(&frame->cb);
    cb_destroy(&frame->compute_cb);
}

VkCommandBuffer frame_cb(struct Frame* frame) {
    cb_reset(&frame->cb);
    return frame->cmd;
}

VkCommandBuffer frame_compute_cb(struct Frame* frame) {
    cb_reset(&frame->compute_cb);
    return frame->compute_cmd;
}
