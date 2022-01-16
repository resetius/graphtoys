#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <render/buffer.h>
#include <render/render.h>

#include "buffer.h"

struct BufferManagerImpl {
    struct BufferManagerBase base;
};

static int create(
    struct BufferManager* mgr,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    const void* data,
    int size)
{
    return -1;
}

static void update(
    struct BufferManager* mgr,
    int id,
    const void* data,
    int offset,
    int size)
{
}

static void release(struct BufferManager* mgr, void* buf1) {
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

    return (struct BufferManager*)b;
}
