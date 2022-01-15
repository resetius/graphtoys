#pragma once

#include <render/buffer.h>

#include "glad/gl.h"

struct BufferImpl {
    GLuint buffer;
    GLenum type;
    GLenum mtype;
    int size;
    int valid;
};

struct Render;
struct BufferManager* buf_mgr_opengl_new(struct Render* r);
// TODO: BufferManager free
