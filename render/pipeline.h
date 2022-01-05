#pragma once

#include <stdint.h>
#include "render.h"

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
        int id,
        int descr_id,
        const void* data,
        int offset,
        int size);

    void (*run)(struct Pipeline*);

    void (*use_texture)(struct Pipeline* p1, void* texture);
    void (*start)(struct Pipeline* p1);
    void (*draw)(struct Pipeline* p1, int buffer_id);
};

struct ShaderCode {
    const char* glsl;
    const char* spir_v;
    int size;
};

struct PipelineBuilder {

    struct PipelineBuilder* (*begin_program)(struct PipelineBuilder*);
    struct PipelineBuilder* (*add_vs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*add_fs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*end_program)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_buffer)(struct PipelineBuilder*, int stride);
    struct PipelineBuilder* (*buffer_data)(struct PipelineBuilder*, const void* data, int size);
    struct PipelineBuilder* (*buffer_dynamic)(struct PipelineBuilder*); // TODO: remove
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
