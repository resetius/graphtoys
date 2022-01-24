#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "buffer.h"
#include "render.h"

struct BufferBase* buffer_acquire_(struct BufferManagerBase* b, int size) {
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
    printf("(%p) buffer memory: %ld\n", b, b->total_memory);
    buf->valid = 1;
    buf->size = size;
    return buf;
}

void buffer_release_(struct BufferManager* mgr, int id) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    struct BufferBase base = {
        .id = id,
        .valid = 0,
        .size = 0,
    };
    assert(id < b->n_buffers);

    b->iface.release(&b->iface, &b->buffers[id*b->buffer_size]);

    b->total_memory -= ((struct BufferBase*)&b->buffers[id*b->buffer_size])->size;
    printf("(%p) buffer memory: %ld\n", b, b->total_memory);

    memcpy(&b->buffers[id*b->buffer_size], &base, sizeof(base));

    while (b->n_buffers > 0 &&
           !((struct BufferBase*)&b->buffers[b->buffer_size * (b->n_buffers - 1)])->valid)
    {
        b->n_buffers --;
    }
}

void buffer_mgr_free_(struct BufferManager* mgr) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    int i;
    for (i = 0; i < b->n_buffers; i++) {
        b->iface.release(&b->iface, b->iface.get(&b->iface, i));
    }
    free(b->buffers);
    free(b);
}

void* buffer_get_(struct BufferManager* mgr, int id) {
    struct BufferManagerBase* b = (struct BufferManagerBase*)mgr;
    assert(id < b->n_buffers);
    return &b->buffers[id*b->buffer_size];
}
