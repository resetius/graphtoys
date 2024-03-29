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

struct Render* rend_opengl_new(struct RenderConfig cfg);
struct Render* rend_vulkan_new(struct RenderConfig cfg);

void rend_free(struct Render*);

struct Texture* rend_tex_new(struct Render*, void* data, enum TexType tex_type);
struct Char* rend_char_new(struct Render*, wchar_t ch, void* face);
