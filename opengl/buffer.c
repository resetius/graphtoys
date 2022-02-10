#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <glad/gl.h>

#include <render/buffer.h>
#include <render/render.h>

#include "buffer.h"

struct BufferManagerImpl {
    struct BufferManagerBase base;
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
    case BUFFER_INDEX:
        ret = GL_ELEMENT_ARRAY_BUFFER;
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
    struct BufferImpl* buf = (struct BufferImpl*)b->base.iface.acquire(mgr, size);
    int btype = buffer_type(type);
    int mtype = memory_type(mem_type);
    buf->type = btype;
    buf->mtype = mtype;
    buf->size = size;
    glGenBuffers(1, &buf->buffer);
    glBindBuffer(buf->type, buf->buffer);
    glBufferData(buf->type, size, data, buf->mtype);
    glBindBuffer(buf->type, 0);
    return buf->base.id;
}

static void update(
    struct BufferManager* mgr,
    int id,
    const void* data,
    int offset,
    int size)
{
    struct BufferImpl* buf = (struct BufferImpl*)mgr->get(mgr, id);
    glBindBuffer(buf->type, buf->buffer);
    glBufferSubData(buf->type, offset, size, data);
    glBindBuffer(buf->type, 0);
}

static void release(struct BufferManager* mgr, void* buf1) {
    struct BufferImpl* buf = buf1;
    if (buf->base.valid) {
        glDeleteBuffers(1, &buf->buffer);
    }
}

struct BufferManager* buf_mgr_opengl_new(struct Render* r) {
    struct BufferManagerImpl* b = calloc(1, sizeof(*b));
    buffermanager_base_ctor(&b->base, sizeof(struct BufferImpl));

    b->base.iface.create = create;
    b->base.iface.release = release;
    b->base.iface.update = update;

    return (struct BufferManager*)b;
}
