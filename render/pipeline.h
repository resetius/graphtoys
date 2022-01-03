#pragma once

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
        const void* data,
        int offset,
        int size);

    void (*run)(struct Pipeline*);
};

struct PipelineBuilder {

    struct PipelineBuilder* (*begin_program)(struct PipelineBuilder*);
    struct PipelineBuilder* (*add_vs)(struct PipelineBuilder*, const char* shader);
    struct PipelineBuilder* (*add_fs)(struct PipelineBuilder*, const char* shader);
    struct PipelineBuilder* (*end_program)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_buffer)(struct PipelineBuilder*, int n_vertices);
    struct PipelineBuilder* (*buffer_size)(struct PipelineBuilder*, int size);
    struct PipelineBuilder* (*buffer_data)(struct PipelineBuilder*, void* data);
    struct PipelineBuilder* (*buffer_dynamic)(struct PipelineBuilder*);
    struct PipelineBuilder* (*buffer_attribute)(
        struct PipelineBuilder*,
        int location,
        int channels, int bytes_per_channel,
        int stride, uint64_t offset);
    struct PipelineBuilder* (*end_buffer)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_uniform)(
        struct PipelineBuilder*,
        int binding,
        const char* blockName,
        int size);
    struct PipelineBuilder* (*end_uniform)(struct PipelineBuilder*);

    struct Pipeline* (*build)(struct PipelineBuilder*); // free
};
