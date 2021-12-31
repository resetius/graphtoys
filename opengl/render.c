#include <stdlib.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <render/render.h>

struct Program* prog_opengl_new();

struct RenderImpl {
    // Char
    GLuint char_vao;
    GLuint char_vbo;
    GLuint char_sampler;
};

struct Program* rend_prog_new(struct Render* r) {
    return prog_opengl_new();
};

struct Render* rend_opengl_new()
{
    struct RenderImpl* r = calloc(1, sizeof(*r));
    return (struct Render*)r;
}
