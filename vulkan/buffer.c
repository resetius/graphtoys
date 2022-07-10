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
    int size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct RenderImpl* r = b->r;
    struct BufferImpl* buf = (struct BufferImpl*)b->base.iface.acquire(mgr, size);
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

    const uint32_t families[] = {r->graphics_family, r->compute_family};
    int n_families = r->graphics_family == r->compute_family
        ? 0
        : 2;

    for (i = 0; i < buf->n_buffers; i++) {
        create_buffer(
            r->memory_properties, r->log_dev,
            buf->size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | vk_type,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            //VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &stagingBuffer,
            &stagingBufferMemory,
            n_families,
            families);

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
            create_buffer(
                r->memory_properties, r->log_dev,
                size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT|vk_type,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &buf->buffer[i],
                &buf->memory[i],
                n_families, families);

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
    int offset,
    int size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)mgr->get(mgr, id);
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
    int offset,
    int size,
    int flags)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = (struct BufferImpl*)mgr->get(mgr, id);
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

    vkCmdPipelineBarrier(
        flags == 0 ? r->buffer : r->compute_buffer,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT|VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        1, &barrier,
        0, NULL,
        0, NULL
        );
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

struct BufferManager* buf_mgr_vulkan_new(struct Render* r) {
    struct BufferManagerImpl* b = calloc(1, sizeof(*b));
    buffermanager_base_ctor(&b->base, sizeof(struct BufferImpl));

    b->base.iface.create = create;
    b->base.iface.release = release;
    b->base.iface.update = update;
    b->base.iface.update_sync = update_sync;

    b->r = (struct RenderImpl*)r;

    return (struct BufferManager*)b;
}
