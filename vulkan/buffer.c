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
    struct BufferImpl* buf = (struct BufferImpl*)buffer_acquire_(&b->base, size);
    int i;

    buf->size = size;
    buf->n_buffers = 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    uint32_t vk_type = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    if (type == BUFFER_UNIFORM) {
        vk_type = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf->n_buffers = r->sc.n_images;
    }

    if (type == BUFFER_SHADER_STORAGE) {
        vk_type |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }

    for (i = 0; i < buf->n_buffers; i++) {
        create_buffer(
            r->phy_dev, r->log_dev,
            buf->size,
            vk_type,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            //VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &stagingBuffer,
            &stagingBufferMemory);

        assert(data || mem_type == MEMORY_DYNAMIC);

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
                r->phy_dev, r->log_dev,
                size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT|vk_type,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &buf->buffer[i],
                &buf->memory[i]);

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
    struct BufferManager iface = {
        .get = buffer_get_,
        .create = create,
        .release = release,
        .update = update,
        .destroy = buffer_release_,
        .free = buffer_mgr_free_,
    };

    b->base.iface = iface;
    b->base.buffer_size = sizeof(struct BufferImpl);
    b->r = (struct RenderImpl*)r;

    return (struct BufferManager*)b;
}
