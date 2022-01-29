#include <stdio.h>
#include <stdlib.h>

#include "commandbuffer.h"
#include "render_impl.h"

void cb_init(struct CommandBuffer* d, struct RenderImpl* r) {
    VkCommandPoolCreateInfo cpInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = r->graphics_family,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
    };

    if (vkCreateCommandPool(r->log_dev, &cpInfo, NULL, &d->pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        exit(-1);
    }

    d->dev = r->log_dev;
}

VkCommandBuffer cb_acquire(struct CommandBuffer* d) {
    VkCommandBuffer buffer;
    VkCommandBufferAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = d->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(d->dev, &info, &buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        exit(-1);
    }
    return buffer;
}

void cb_destroy(struct CommandBuffer* d) {
	vkDestroyCommandPool(d->dev, d->pool, NULL);
}

void cb_begin(struct CommandBuffer* d, VkCommandBuffer buffer) {
    VkCommandBufferBeginInfo cbBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = NULL
    };

	if (vkBeginCommandBuffer(buffer, &cbBeginInfo) != VK_SUCCESS) {
		fprintf(stderr, "Failed to begin command buffer\n");
        exit(-1);
	}
}

void cb_end(struct CommandBuffer* d, VkCommandBuffer buffer) {
    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to record command buffer");
        exit(-1);
    }
}

void cb_reset(struct CommandBuffer* d) {
    vkResetCommandPool(d->dev, d->pool, 0);
}
