#pragma once

#include <stddef.h>

struct Program;
struct Texture;
struct Char;
struct PipelineBuilder;
struct BufferManager;

struct RenderConfig {
    const char* api;
    // 0 - off
    // 1 - on
    // 2 - adaptive
    int vsync;
    int show_fps;
    int triple_buffer;
};

struct Render {
    struct Program* (*prog_new)(struct Render*);
    struct PipelineBuilder* (*pipeline)(struct Render*);
    struct BufferManager* (*buffer_manager)(struct Render*);
    struct Mesh* (*mesh_new)(struct Render*);
    struct Texture* (*tex_new)(struct Render*);
    struct Char* (*char_new)(struct Render*, wchar_t ch, void* face);
    void (*free)(struct Render*);

    void (*init)(struct Render*);
    void (*set_view_entity)(struct Render*, void* );
    void (*draw_begin)(struct Render*);
    void (*draw_end)(struct Render*);
    void (*set_viewport)(struct Render*, int w, int h);
};

struct Render* rend_opengl_new(struct RenderConfig cfg);
struct Render* rend_vulkan_new(struct RenderConfig cfg);

void rend_free(struct Render*);

struct Program* rend_prog_new(struct Render*);
struct Mesh* rend_mesh_new(struct Render*);
struct Texture* rend_tex_new(struct Render*);
struct Char* rend_char_new(struct Render*, wchar_t ch, void* face);
