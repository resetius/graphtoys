#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <render/buffer.h>
#include <render/render.h>

struct BufferManagerImpl {
    struct BufferManager base;
};

struct BufferManager* buf_mgr_opengl_new(struct Render* r) {
    struct BufferManagerImpl* b = calloc(1, sizeof(*b));
    struct BufferManager base = {
        .get = get,
        .create = create,
        .update = update,
        .destroy = destroy,
        .free = mgr_free,
    };

    b->base = base;

    return (struct BufferManager*)b;
}
