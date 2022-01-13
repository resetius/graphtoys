#pragma once

#include <stdint.h>
#include "render.h"

enum BufferType {
    BUFFER_ARRAY, // vao
    BUFFER_SHADER_STORAGE
};

enum BufferMemoryType {
    MEMORY_STATIC,
    MEMORY_DYNAMIC,
    MEMORY_DYNAMIC_COPY
};

struct Pipeline {

    void (*free)(struct Pipeline*);

    void (*uniform_update)(
        struct Pipeline*,
        int id,
        const void* data,
        int offset,
        int size);

    void (*buffer_update)(
        struct Pipeline*,
        int buffer_od,
        const void* data,
        int offset,
        int size);

    int (*buffer_create)(
        struct Pipeline* p1,
        enum BufferType type,
        enum BufferMemoryType mem_type,
        int binding,
        const void* data,
        int size); // -> buffer id

    void (*use_texture)(struct Pipeline* p1, void* texture);
    void (*start)(struct Pipeline* p1);
    void (*draw)(struct Pipeline* p1, int buffer_id);
};

void pl_free(struct Pipeline*);
void pl_uniform_update(struct Pipeline*, int id, const void* data, int offset, int size);
void pl_buffer_update(struct Pipeline*, int od, const void* data, int offset, int size);
void pl_buffer_create(struct Pipeline*, enum BufferType type, enum BufferMemoryType mtype,
                      int binding, const void* data, int size);
void pl_use_texture(struct Pipeline* p1, void* texture);
void pl_start(struct Pipeline*);
void pl_draw(struct Pipeline* p1, int buffer_id);

struct ShaderCode {
    const char* glsl;
    const char* spir_v;
    int size;
};

struct PipelineBuilder {

    struct PipelineBuilder* (*begin_program)(struct PipelineBuilder*);
    struct PipelineBuilder* (*add_vs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*add_fs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*add_cs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*end_program)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_buffer)(struct PipelineBuilder*, int stride);
    struct PipelineBuilder* (*buffer_attribute)(
        struct PipelineBuilder*,
        int location,
        int channels, int bytes_per_channel,
        uint64_t offset);
    struct PipelineBuilder* (*end_buffer)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_uniform)(
        struct PipelineBuilder*,
        int binding,
        const char* blockName,
        int size);
    struct PipelineBuilder* (*end_uniform)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_sampler)(struct PipelineBuilder* p1, int binding);
    struct PipelineBuilder* (*end_sampler)(struct PipelineBuilder* p1);

    struct PipelineBuilder* (*enable_depth)(struct PipelineBuilder* p1);
    struct PipelineBuilder* (*enable_blend)(struct PipelineBuilder* p1);

    struct Pipeline* (*build)(struct PipelineBuilder*); // free
};
