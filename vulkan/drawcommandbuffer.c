#include <stdio.h>
#include <stdlib.h>

#include "drawcommandbuffer.h"
#include "render_impl.h"

void dcb_init(struct DrawCommandBuffer* d, struct RenderImpl* r) {
    VkCommandPoolCreateInfo cpInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = r->graphics_family,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    };

    if (vkCreateCommandPool(r->log_dev, &cpInfo, NULL, &d->pool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        exit(-1);
    }

    d->n_buffers = r->sc.n_images;
    d->buffers = malloc(d->n_buffers*sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo cbInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = d->pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32_t)d->n_buffers
    };

    if (vkAllocateCommandBuffers(r->log_dev, &cbInfo, d->buffers) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate command buffers\n");
        exit(-1);
    }
    d->dev = r->log_dev;
}

void dcb_destroy(struct DrawCommandBuffer* d) {
    free(d->buffers);
	vkDestroyCommandPool(d->dev, d->pool, NULL);
}
