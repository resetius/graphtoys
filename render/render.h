#pragma once

#include <stdint.h>
#include <stddef.h>

struct Texture;
struct Char;
struct PipelineBuilder;
struct BufferManager;
struct Config;

struct RenderConfig {
    const char* api;
    // 0 - off
    // 1 - on
    // 2 - adaptive
    int vsync;
    int show_fps;
    int triple_buffer;
    int fullscreen;
    int vidmode;
    int width;
    int height;
    float clear_color[4];

    struct Config* cfg;
};

enum TexType {
    TEX_KTX = 0
};

enum CounterType {
    COUNTER_COMPUTE = 0,
    COUNTER_VERTEX = 1,
    COUNTER_FRAG = 2,
};

struct Counter {
    const char* name;
    uint64_t value;
    uint64_t count;
    enum CounterType type;
};

struct Render {
    const struct RenderVtbl* vtbl;
};

struct RenderVtbl {
    struct PipelineBuilder* (*pipeline)(struct Render*);
    struct BufferManager* (*buffer_manager)(struct Render*);
    struct Texture* (*tex_new)(struct Render*, void* data, enum TexType tex_type);
    struct Char* (*char_new)(struct Render*, wchar_t ch, void* face);
    void (*free)(struct Render*);

    void (*init)(struct Render*);
    void (*set_view_entity)(struct Render*, void* );
    void (*draw_begin)(struct Render*);
    void (*draw_end)(struct Render*);
    void (*set_viewport)(struct Render*, int w, int h);

    void (*print_compute_stats)(struct Render*);
    int (*counter_new)(struct Render*, const char* name, enum CounterType counter_type);
    void (*counter_submit)(struct Render*, int id);

    void (*screenshot)(struct Render*, void** data, int* w, int* h);
};

struct Render* render_opengl_new(struct RenderConfig cfg);
struct Render* render_vulkan_new(struct RenderConfig cfg);

#define render_pipeline(r) ((r)->vtbl->pipeline(r))
#define render_buffer_manager(r) ((r)->vtbl->buffer_manager(r))
#define render_tex_new(r, data, tex_type) ((r)->vtbl->tex_new((r), (data), (tex_type)))
#define render_char_new(r, ch, face) ((r)->vtbl->char_new((r), (ch), (face)))
#define render_free(r) ((r) ? (r)->vtbl->free(r) : (void)0)
#define render_init(r) ((r)->vtbl->init(r))
#define render_set_view_entity(r, e) ((r)->vtbl->set_view_entity((r), (e)))
#define render_draw_begin(r) ((r)->vtbl->draw_begin(r))
#define render_draw_end(r) ((r)->vtbl->draw_end(r))
#define render_set_viewport(r, w, h) ((r)->vtbl->set_viewport((r), (w), (h)))
#define render_print_compute_stats(r) ((r)->vtbl->print_compute_stats(r))
#define render_counter_new(r, name, type) ((r)->vtbl->counter_new((r), (name), (type)))
#define render_counter_submit(r, id) ((r)->vtbl->counter_submit((r), (id)))
#define render_screenshot(r, data, w, h) ((r)->vtbl->screenshot((r), (data), (w), (h)))
