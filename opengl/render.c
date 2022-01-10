#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <render/render.h>
#include <render/program.h>
#include <render/char.h>
#include <render/pipeline.h>

#include <freetype/freetype.h>

struct Program* prog_opengl_new();

struct CharImpl {
    struct Char base;
    GLuint tex_id;
};

struct RenderImpl {
    struct Render base;
    struct RenderConfig cfg;
    GLFWwindow* window;
};

static struct Program* rend_prog_new_(struct Render* r) {
    return prog_opengl_new();
};

static void* char_texture_(struct Char* ch) {
    return & (((struct CharImpl*)ch)->tex_id);
}

static void char_free_(struct Char* ch) {
    if (ch) {
        struct CharImpl* c = (struct CharImpl*)ch;
        glDeleteTextures(1, &c->tex_id);
        free(c);
    }
}

static struct Char* rend_char_new_(struct Render* r, wchar_t ch, void* bm) {
    struct CharImpl* c = calloc(1, sizeof(*c));
    FT_Face face = (FT_Face)bm;
    FT_Bitmap bitmap = face->glyph->bitmap;
    struct Char base = {
        .texture = char_texture_,
        .free = char_free_
    };
    c->base = base;
    c->base.ch = ch;
    c->base.w = bitmap.width;
    c->base.h = bitmap.rows;
    c->base.left = face->glyph->bitmap_left;
    c->base.top = face->glyph->bitmap_top;
    c->base.advance = face->glyph->advance.x;

    if (bitmap.width*bitmap.rows == 0) {
        free(c);
        return NULL;
    }

    glGenTextures(1, &c->tex_id);

    glBindTexture(GL_TEXTURE_2D, c->tex_id);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap.pitch);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RED,
        bitmap.width,
        bitmap.rows, 0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        bitmap.buffer);

    return (struct Char*)c;
};

static void rend_free_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    free(r);
}

static void set_view_entity_(struct Render* r, void* data) {
    GLFWwindow* window = data;
    ((struct RenderImpl*)r)->window = window;
    glfwMakeContextCurrent(window);
}

static void set_viewport(struct Render* r, int w, int h) {
    glViewport(0, 0, w, h);
}

static void draw_begin_(struct Render* r) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void draw_end_(struct Render* r) {
    glfwSwapBuffers(((struct RenderImpl*)r)->window);
}

static void gl_info() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    //GLint count = 0;
    //glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
    //printf("Binary formats: %d\n", count);
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    gladLoadGL(glfwGetProcAddress);

//    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    gl_info();

    glfwSwapInterval(r->cfg.vsync != 0); // vsync
}

struct PipelineBuilder* pipeline_builder_opengl(struct Render*);

struct Render* rend_opengl_new(struct RenderConfig cfg)
{
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = rend_free_,
        .char_new = rend_char_new_,
        .prog_new = rend_prog_new_,
        .set_view_entity = set_view_entity_,
        .draw_begin = draw_begin_,
        .draw_end = draw_end_,
        .init = init_,
        .pipeline = pipeline_builder_opengl,
        .set_viewport = set_viewport,
    };
    r->base = base;
    r->cfg = cfg;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac os compatible
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return (struct Render*)r;
}
