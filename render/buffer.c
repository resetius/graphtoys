#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"
#include "render.h"

static struct BufferBase* buffer_acquire_(struct BufferManager* bm, int size) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)bm;
    struct BufferBase* buf = NULL;
    if (b->n_buffers >= b->cap) {
        int i;
        for (i = b->n_buffers - 1; i >= 0 && ((struct BufferBase*)&b->buffers[b->buffer_size * i])->valid; --i) {
            ;
        }
        if (i >= 0) {
            buf = (struct BufferBase*)&b->buffers[i*b->buffer_size];
        }
    }
    if (!buf && b->n_buffers >= b->cap) {
        int old_cap = b->cap;
        b->cap = (1+b->cap)*2;
        b->buffers = realloc(b->buffers, b->cap*b->buffer_size);
        memset(b->buffers + old_cap * b->buffer_size,
               0,
               (b->cap - old_cap)*b->buffer_size);
    }

    if (!buf) {
        int id = b->n_buffers++;
        buf = (struct BufferBase*)&b->buffers[id*b->buffer_size];
        buf->id = id;
    }
    b->total_memory += size;
    printf("(%p) buffer memory: %lld +%lld\n", b, (long long)b->total_memory, (long long)size);
    buf->valid = 1;
    buf->size = size;
    return buf;
}

static void buffer_release_(struct BufferManager* mgr, int id) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    struct BufferBase base = {
        .id = id,
        .valid = 0,
        .size = 0,
    };
    assert(id < b->n_buffers);

    b->iface.release(&b->iface, &b->buffers[id*b->buffer_size]);

    b->total_memory -= ((struct BufferBase*)&b->buffers[id*b->buffer_size])->size;
    printf("(%p) free buffer memory: %lld\n", b, (long long)b->total_memory);

    memcpy(&b->buffers[id*b->buffer_size], &base, sizeof(base));

    while (b->n_buffers > 0 &&
           !((struct BufferBase*)&b->buffers[b->buffer_size * (b->n_buffers - 1)])->valid)
    {
        b->n_buffers --;
    }
}

static void buffer_mgr_free_(struct BufferManager* mgr) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    while (b->n_buffers > 0) {
        buffer_release_(mgr, b->n_buffers-1);
    }
    free(b->buffers);
    free(b);
}

static void* buffer_get_(struct BufferManager* mgr, int id) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    assert(id < b->n_buffers);
    return &b->buffers[id*b->buffer_size];
}

int buffer_create(
    struct BufferManager* b,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    const void* data,
    int size)
{
    return b->create(b, type, mem_type, data, size);
}

void buffer_update(
    struct BufferManager* b,
    int id,
    const void* data,
    int offset,
    int size)
{
    return b->update(b, id, data, offset, size);
}

void buffermanager_base_ctor(struct BufferManagerBase* bm, int buffer_size) {
    struct BufferManager iface = {
        .get = buffer_get_,
        .destroy = buffer_release_,
        .free = buffer_mgr_free_,
        .acquire = buffer_acquire_,
    };

    bm->iface = iface;
    bm->buffer_size = buffer_size;
}
