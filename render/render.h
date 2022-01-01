#pragma once

#include <ctype.h>

struct Program;
struct Texture;
struct Char;

struct Render {
    struct Program* (*prog_new)(struct Render*);
    struct Mesh* (*mesh_new)(struct Render*);
    struct Texture* (*tex_new)(struct Render*);
    struct Char* (*char_new)(struct Render*, wchar_t ch, void* face);
    void (*free)(struct Render*);

    void (*init)(struct Render*);
    void (*set_view_entity)(struct Render*, void* );
    void (*draw_begin)(struct Render*, int* w, int* h);
    void (*draw_ui)(struct Render*);
    void (*draw_end)(struct Render*);
};

struct Render* rend_opengl_new();
struct Render* rend_vulkan_new();

void rend_free(struct Render*);

struct Program* rend_prog_new(struct Render*);
struct Mesh* rend_mesh_new(struct Render*);
struct Texture* rend_tex_new(struct Render*);
struct Char* rend_char_new(struct Render*, wchar_t ch, void* face);
