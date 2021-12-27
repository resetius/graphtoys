#include <stdlib.h>
#include <stdio.h>
#include <freetype/freetype.h>
#include "font.h"
#include "font_vs.h"
#include "font_fs.h"
#include "mesh.h"
#include "program.h"
#include "RobotoMono-Regular.h"

struct Char {
    GLuint tex_id;
    wchar_t ch;
    int w;
    int h;
};

struct FontImpl {
    struct Font base;
    struct Mesh* m;
    struct Program* p;
    struct Char chars[65536];
    FT_Library library;
    FT_Face face;
    GLuint sampler;
};

static void char_load(struct FontImpl* f, wchar_t ch) {
    struct Char* out = &f->chars[ch];
    int error;
    error = FT_Load_Char(f->face, ch, FT_LOAD_RENDER);
    if (error) {
        printf("Cannot load char: %d\n", error);
        exit(1);
    }
    error = FT_Render_Glyph(f->face->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        printf("Cannot render glyph: %d\n", error);
        exit(1);
    }

    FT_Bitmap bitmap = f->face->glyph->bitmap;
    glGenTextures(1, &out->tex_id);

    glBindTexture(GL_TEXTURE_2D, out->tex_id);
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
}

static void font_render(struct Font* o, float x, float y, const char* string, float ratio) {
    struct FontImpl* f = (struct FontImpl*)o;

    mat4x4 m, v, mv, p, mvp;
    mat4x4_identity(m);
    mat4x4_ortho(p, -ratio, ratio, 1.f, -1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    //vec3 eye = {.0f, .0f, -1.f};
    //vec3 center = {.0f, .0f, .0f};
    //vec3 up = {.0f, 1.f, .0f};
    //mat4x4_look_at(v, eye, center, up);
    //mat4x4_mul(mv, v, m);
    //mat4x4_perspective(p, 70./2./M_PI, ratio, 0.3f, 100.f);
    //mat4x4_mul(mvp, p, mv);

    prog_use(f->p);
    prog_set_mat4x4(f->p, "MVP", &mvp);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, f->chars['a'].tex_id);

    glBindSampler(0, f->sampler);

    mesh_render(f->m);
}

struct Font* font_new() {
    struct FontImpl* t = calloc(1, sizeof(*t));
    int i;

    // {-1, 1} {1, 1}
    // {-1,-1} {1, -1}
    struct TexVertex1 vertices[] = {
        {{-1.0f, 1.0f, 0.0f}, { 0.0f, 1.0f}},
        {{-1.0f,-1.0f, 0.0f}, { 0.0f, 0.0f}},
        {{ 1.0f,-1.0f, 0.0f}, { 1.0f, 0.0f}},

        {{-1.0f, 1.0f, 0.0f}, { 0.0f, 1.0f}},
        {{ 1.0f,-1.0f, 0.0f}, { 1.0f, 0.0f}},
        {{ 1.0f, 1.0f, 0.0f}, { 1.0f, 1.0f}}
    };

    struct Font base = {
        .render = font_render
    };
    t->base = base;

    int error = FT_Init_FreeType(&t->library);
    if (error) {
        printf("Cannot load FreeType\n");
        exit(1);
    }
    error = FT_New_Memory_Face(
        t->library,
        (const FT_Byte*)font_RobotoMono_Regular,
        font_RobotoMono_Regular_size,
        0,
        &t->face);
    if (error) {
        printf("Cannot load Font\n");
        exit(1);
    }
    error = FT_Set_Char_Size(
          t->face,    /* handle to face object           */
          0,       /* char_width in 1/64th of points  */
          16*64,   /* char_height in 1/64th of points */
          300,     /* horizontal device resolution    */
          300 );   /* vertical device resolution      */
    if (error) {
        printf("Cannot set char size\n");
        exit(1);
    }

    t->m = mesh_tex1_new(vertices, 6, 0, 1);
    t->p = prog_new();

    prog_add_vs(t->p, font_font_vs);
    prog_add_fs(t->p, font_font_fs);
    prog_link(t->p);

    glGenSamplers(1, &t->sampler);
    glSamplerParameteri(t->sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(t->sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(t->sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(t->sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    vec4 border = {0.0f,0.0f,0.0f,0.5f};
    glSamplerParameterfv(t->sampler, GL_TEXTURE_BORDER_COLOR, border);

    GLint location = glGetUniformLocation(t->p->program, "Texture");
    if (location < 0) {
        printf("WTF?\n");
        exit(1);
    }
    glUniform1i(location, 0);

    memset(t->chars, 0, sizeof(t->chars));
    for (i = 'a'; i <= 'z'; i++) {
        char_load(t, i);
    }
    for (i = 'A'; i <= 'Z'; i++) {
        char_load(t, i);
    }

    return (struct Font*)t;
}

void font_free(struct Font* o) {
    int i;
    struct FontImpl* f = (struct FontImpl*)o;
    for (i = 0; i < sizeof(f->chars)/sizeof(struct Char); i++) {
        if (f->chars[i].ch) {
            glDeleteTextures(1, &f->chars[i].tex_id);
        }
    }
    mesh_free(f->m);
    prog_free(f->p);
    free(f);
}
