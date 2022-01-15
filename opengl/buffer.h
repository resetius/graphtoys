#pragma once

#include <render/buffer.h>

struct BufferImpl {
    GLuint buffer;
    GLenum type;
    GLenum mtype;
    int size;
    int valid;
};

struct Render;
struct BufferManager* buf_mgr_opengl_new(struct Render* r);
