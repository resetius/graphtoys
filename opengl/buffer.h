#pragma once

#include <render/buffer.h>

#include "glad/gl.h"

struct BufferImpl {
    struct BufferBase base;
    GLuint buffer;
    GLenum type;
    GLenum mtype;
    int size;
};

struct Render;
struct BufferManager* buf_mgr_opengl_new(struct Render* r);
// TODO: BufferManager free
