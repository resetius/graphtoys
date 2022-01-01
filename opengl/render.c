#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <render/render.h>
#include <render/program.h>
#include <render/char.h>

#include <freetype/freetype.h>

struct Program* prog_opengl_new();

struct CharImpl {
    struct Char base;
    GLuint vao;
    GLuint vbo;
    GLuint sampler;
    GLuint tex_id;
};

struct RenderImpl {
    struct Render base;
    GLFWwindow* window;

    // Char
    GLuint char_vao;
    GLuint char_vbo;
    GLuint char_sampler;
};

static struct Program* rend_prog_new_(struct Render* r) {
    return prog_opengl_new();
};

static void init_char_render(struct RenderImpl* r) {
    glGenVertexArrays(1, &r->char_vao);
    glGenBuffers(1, &r->char_vbo);
    glBindVertexArray(r->char_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->char_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 *
                          sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenSamplers(1, &r->char_sampler);
    glSamplerParameteri(r->char_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(r->char_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(r->char_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(r->char_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    vec4 border = {0.0f,0.0f,0.0f,0.5f};
    glSamplerParameterfv(r->char_sampler, GL_TEXTURE_BORDER_COLOR, border);
}

static void char_render_(struct Char* ch, int x, int y) {
    GLuint tex_id = ((struct CharImpl*)ch)->tex_id;
    GLuint sampler = ((struct CharImpl*)ch)->sampler;
    GLuint vao = ((struct CharImpl*)ch)->vao;
    GLuint vbo = ((struct CharImpl*)ch)->vbo;
    float xpos, ypos, w, h;
    xpos = x + ch->left;
    ypos = y - (ch->h - ch->top);
    w = ch->w;
    h = ch->h;

    float vertices[6][4] = {
        { xpos, ypos + h,     0.0, 0.0 },
        { xpos, ypos,         0.0, 1.0 },
        { xpos + w, ypos,     1.0, 1.0 },

        { xpos, ypos + h,     0.0, 0.0 },
        { xpos + w, ypos,     1.0, 1.0 },
        { xpos + w, ypos + h, 1.0, 0.0 }
    };

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glBindSampler(0, sampler);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices),
                    vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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
        .render = char_render_,
        .free = char_free_
    };
    c->base = base;
    glGenTextures(1, &c->tex_id);
    c->base.ch = ch;
    c->base.w = bitmap.width;
    c->base.h = bitmap.rows;
    c->base.left = face->glyph->bitmap_left;
    c->base.top = face->glyph->bitmap_top;
    c->base.advance = face->glyph->advance.x;

    c->vao = ((struct RenderImpl*)r)->char_vao;
    c->vbo = ((struct RenderImpl*)r)->char_vbo;
    c->sampler = ((struct RenderImpl*)r)->char_sampler;

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
    glDeleteBuffers(1, &r->char_vbo);
    glDeleteVertexArrays(1, &r->char_vao);
    free(r);
}

static void set_view_entity_(struct Render* r, void* data) {
    GLFWwindow* window = data;
    ((struct RenderImpl*)r)->window = window;
    glfwMakeContextCurrent(window);
}

static void draw_begin_(struct Render* r, int* w, int* h) {
    glfwGetFramebufferSize(((struct RenderImpl*)r)->window, w, h);
    glViewport(0, 0, *w, *h);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

static void draw_ui_(struct Render* r) {
    glDisable(GL_DEPTH_TEST);
}

static void draw_end_(struct Render* r) {
    glfwSwapBuffers(((struct RenderImpl*)r)->window);
}

static void gl_info() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    gladLoadGL(glfwGetProcAddress);

    init_char_render(r);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    gl_info();

    glfwSwapInterval(1); // vsync
}

struct Render* rend_opengl_new()
{
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = rend_free_,
        .char_new = rend_char_new_,
        .prog_new = rend_prog_new_,
        .set_view_entity = set_view_entity_,
        .draw_begin = draw_begin_,
        .draw_end = draw_end_,
        .draw_ui = draw_ui_,
        .init = init_
    };
    r->base = base;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); // mac os compatible
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return (struct Render*)r;
}
