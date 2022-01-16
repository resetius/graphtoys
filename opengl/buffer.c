#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <glad/gl.h>

#include <render/buffer.h>
#include <render/render.h>

#include "buffer.h"

struct BufferManagerImpl {
    struct BufferManager base;
    struct BufferImpl* buffers;
    int n_buffers;
    int cap;
    int valid;
};

static GLenum buffer_type(enum BufferType type) {
    GLenum ret = -1;
    switch (type) {
    case BUFFER_ARRAY:
        ret = GL_ARRAY_BUFFER;
        break;
    case BUFFER_SHADER_STORAGE:
        ret = GL_SHADER_STORAGE_BUFFER;
        break;
    case BUFFER_UNIFORM:
        ret = GL_UNIFORM_BUFFER;
        break;
    default:
        assert(0);
        break;
    }
    return ret;
}

static GLenum memory_type(enum BufferMemoryType type) {
    GLenum ret = -1;
    switch (type) {
    case MEMORY_STATIC:
        ret = GL_STATIC_DRAW;
        break;
    case MEMORY_DYNAMIC:
        ret = GL_DYNAMIC_DRAW;
        break;
    case MEMORY_DYNAMIC_COPY:
        ret = GL_DYNAMIC_COPY;
        break;
    default:
        assert(0);
        break;
    }
    return ret;
}

static int create(
    struct BufferManager* mgr,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    const void* data,
    int size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    struct BufferImpl* buf = NULL;
    int id, btype, mtype;
    if (b->n_buffers >= b->cap) {
        int i;
        for (i = b->n_buffers - 1; i >= 0 && b->buffers[i].valid; --i) {
            ;
        }
        if (i >= 0) {
            buf = &b->buffers[i];
        }
    }
    if (!buf && b->n_buffers >= b->cap) {
        // TODO: generic part
        int old_cap = b->cap;
        b->cap = (1+b->cap)*2;
        b->buffers = realloc(b->buffers, b->cap*sizeof(struct BufferImpl));
        memset(b->buffers + old_cap, 0, (b->cap - old_cap)*sizeof(struct BufferImpl));
    }
    id = b->n_buffers++;
    buf = &b->buffers[id];
    btype = buffer_type(type);
    mtype = memory_type(mem_type);
    memset(buf, 0, sizeof(*buf));
    buf->type = btype;
    buf->mtype = mtype;
    buf->valid = 1;
    glGenBuffers(1, &buf->buffer);
    glBindBuffer(buf->type, buf->buffer);
    glBufferData(buf->type, size, data, buf->mtype);
    glBindBuffer(buf->type, 0);
    return id;
}

static void update(
    struct BufferManager* mgr,
    int id,
    const void* data,
    int offset,
    int size)
{
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    assert(id < b->n_buffers);
    glBindBuffer(b->buffers[id].type, b->buffers[id].buffer);
    glBufferSubData(b->buffers[id].type, offset, size, data);
    glBindBuffer(b->buffers[id].type, 0);
}

static void destroy(struct BufferManager* mgr, int id) {
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    assert(id < b->n_buffers);
    glDeleteBuffers(1, &b->buffers[id].buffer);
    b->buffers[id].valid = 0;
    if (id == b->n_buffers - 1) {
        b->n_buffers --;
    }
}

static void mgr_free(struct BufferManager* mgr) {
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    int i;
    for (i = 0; i < b->n_buffers; i++) {
        if (b->buffers[i].valid) {
            glDeleteBuffers(1, &b->buffers[i].buffer);
        }
    }
    free(b->buffers);
    free(mgr);
}

static void* get(struct BufferManager* mgr, int id) {
    // TODO: generic part
    struct BufferManagerImpl* b = (struct BufferManagerImpl*)mgr;
    assert(id < b->n_buffers);
    return &b->buffers[id];
}

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
