#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <render/buffer.h>
#include <render/render.h>

#include "render_impl.h"
#include "buffer.h"
#include "tools.h"

struct BufferManagerImpl {
    struct BufferManagerBase base;
    struct RenderImpl* r;
};

static int create(
    struct BufferManager* mgr,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    const void* data,
    size_t size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct RenderImpl* r = b->r;
    struct BufferImpl* buf = (struct BufferImpl*)buffer_acquire(mgr, size);
    int i;

    buf->size = size;
    buf->n_buffers = 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    uint32_t vk_type = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (type == BUFFER_UNIFORM) {
        vk_type = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf->n_buffers = r->sc.n_images;
    }

    if (type == BUFFER_SHADER_STORAGE) {
        // TODO: strange or
        vk_type |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    if (type == BUFFER_INDEX) {
        vk_type = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    // CONCURRENT for both families when they differ: compute and graphics use the
    // buffers without queue family ownership transfers.
    const uint32_t families[] = {r->graphics_family, r->compute_family};
    int n_families = r->graphics_family == r->compute_family
        ? 0
        : 2;

    for (i = 0; i < buf->n_buffers; i++) {
        create_buffer(
            r->memory_properties, r->log_dev,
            buf->size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT | vk_type,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            //VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &stagingBuffer,
            &stagingBufferMemory,
            n_families,
            families,
            &buf->memory_flags);

        if (data) {
            void* dst;
            vkMapMemory(r->log_dev, stagingBufferMemory, 0, size, 0, &dst);
            memcpy(dst, data, size);
            vkUnmapMemory(r->log_dev, stagingBufferMemory);
        }

        if (mem_type == MEMORY_DYNAMIC) {
            buf->buffer[i] = stagingBuffer;
            buf->memory[i] = stagingBufferMemory;
        } else {
            // TRANSFER_SRC: read copies the buffer to a host-visible staging buffer.
            create_buffer(
                r->memory_properties, r->log_dev,
                size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT|vk_type,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &buf->buffer[i],
                &buf->memory[i],
                n_families, families,
                &buf->memory_flags);

            copy_buffer(
                r->graphics_family,
                r->g_queue, r->log_dev,
                stagingBuffer,
                buf->buffer[i], size);

            vkDestroyBuffer(r->log_dev, stagingBuffer, NULL);
            vkFreeMemory(r->log_dev, stagingBufferMemory, NULL);
        }
    }

    return buf->base.id;
}

static void update(
    struct BufferManager* mgr,
    int id,
    const void* data,
    size_t offset,
    size_t size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)buffer_get(mgr, id);
    struct RenderImpl* r = b->r;
    void* dest;
    int i = (buf->n_buffers == 1)
        ? 0
        : r->image_index;

    vkMapMemory(r->log_dev, buf->memory[i], offset, size, 0, &dest);
    memcpy(dest, data, size);
    vkUnmapMemory(r->log_dev, buf->memory[i]);
}


static void update_sync(
    struct BufferManager* mgr,
    int id,
    const void* data,
    size_t offset,
    size_t size,
    int flags)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)buffer_get(mgr, id);
    struct RenderImpl* r = b->r;
    int i = (buf->n_buffers == 1)
        ? 0
        : r->image_index;

    vkCmdUpdateBuffer(
        flags == 0 ? r->buffer : r->compute_buffer,
        buf->buffer[i],
        offset,
        size,
        data);

    VkMemoryBarrier barrier = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        NULL,
        VK_ACCESS_MEMORY_WRITE_BIT,
        VK_ACCESS_MEMORY_READ_BIT
    };

    // vkCmdUpdateBuffer is a transfer write; the compute queue may not support graphics stages.
    vkCmdPipelineBarrier(
        flags == 0 ? r->buffer : r->compute_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        flags == 0 ? VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT : VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1, &barrier,
        0, NULL,
        0, NULL
        );
}

// Like glGetBufferSubData: the result includes the compute work recorded so far in
// this frame, so that work is submitted and waited for first.
static void read(
    struct BufferManager* mgr,
    int id,
    void* data,
    size_t offset,
    size_t size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)buffer_get(mgr, id);
    struct RenderImpl* r = b->r;
    int i = (buf->n_buffers == 1)
        ? 0
        : r->image_index;
    VkDeviceMemory memory = buf->memory[i];
    VkMemoryPropertyFlags flags = buf->memory_flags;
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    void* src;

    if (!r->in_frame) {
        fprintf(stderr, "Vulkan buffer read is supported only between draw_begin and draw_end\n");
        exit(-1);
    }

    VkMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
    };

    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        vkCmdPipelineBarrier(
            r->compute_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &barrier, 0, NULL, 0, NULL);
    } else {
        // Used only on the compute queue, so no concurrent sharing.
        create_buffer(
            r->memory_properties, r->log_dev,
            size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &staging,
            &staging_memory,
            0, NULL,
            &flags);

        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(
            r->compute_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 1, &barrier, 0, NULL, 0, NULL);

        VkBufferCopy region = {
            .srcOffset = offset,
            .dstOffset = 0,
            .size = size
        };
        vkCmdCopyBuffer(r->compute_buffer, buf->buffer[i], staging, 1, &region);

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        vkCmdPipelineBarrier(
            r->compute_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_HOST_BIT,
            0, 1, &barrier, 0, NULL, 0, NULL);

        memory = staging_memory;
        offset = 0;
    }

    vk_compute_flush(r);

    // The whole range keeps the invalidate aligned to nonCoherentAtomSize.
    vkMapMemory(r->log_dev, memory, 0, VK_WHOLE_SIZE, 0, &src);
    if (!(flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        VkMappedMemoryRange range = {
            .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
            .memory = memory,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };
        vkInvalidateMappedMemoryRanges(r->log_dev, 1, &range);
    }
    memcpy(data, (char*)src + offset, size);
    vkUnmapMemory(r->log_dev, memory);

    if (staging != VK_NULL_HANDLE) {
        vkDestroyBuffer(r->log_dev, staging, NULL);
        vkFreeMemory(r->log_dev, staging_memory, NULL);
    }
}

static void release(struct BufferManager* mgr, void* buf1) {
    int i;
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)buf1;
    struct RenderImpl* r = b->r;
    for (i = 0; i < buf->n_buffers; i++) {
        vkDestroyBuffer(r->log_dev, buf->buffer[i], NULL);
        vkFreeMemory(r->log_dev, buf->memory[i], NULL);
    }
}

static const struct BufferManagerVtbl buffer_vtbl = {
    .create = create,
    .release = release,
    .update = update,
    .update_sync = update_sync,
    .read = read,
};

struct BufferManager* buf_mgr_vulkan_new(struct Render* r) {
    struct BufferManagerImpl* b = calloc(1, sizeof(*b));
    buffermanager_base_ctor(&b->base, sizeof(struct BufferImpl));
    b->base.iface.vtbl = &buffer_vtbl;

    b->r = (struct RenderImpl*)r;

    return (struct BufferManager*)b;
}
