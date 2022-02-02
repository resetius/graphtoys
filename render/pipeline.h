#pragma once

#include <stdint.h>

#include "render.h"
#include "buffer.h" // TODO: remove

enum GeometryType {
    GEOM_TRIANGLES = 0,
    GEOM_POINTS
};

enum DataType {
    DATA_FLOAT = 0,
    DATA_INT
};

enum CullType {
    CULL_FRONT = 1,
    CULL_BACK = 2,
    CULL_BOTH = 3
};

struct Pipeline {

    void (*free)(struct Pipeline*);

    void (*storage_assign)(struct Pipeline* p1, int storage_id, int buffer_id);
    void (*uniform_assign)(struct Pipeline* p1, int uniform_id, int buffer_id);
    int (*buffer_assign)(struct Pipeline* p1, int descriptor_id, int buffer_id);

    // deprecated
    void (*uniform_update)(
        struct Pipeline*,
        int id,
        const void* data,
        int offset,
        int size);

    void (*storage_swap)(struct Pipeline* p1,
                         int dst, int src);

    void (*use_texture)(struct Pipeline* p1, void* texture);

    void (*start_compute)(struct Pipeline* p1, int sx, int sy, int sz);

    void (*start)(struct Pipeline* p1);
    void (*draw)(struct Pipeline* p1, int buffer_id);
    void (*draw_indexed)(struct Pipeline* p1, int buffer_id, int buffer_index_id, int index_byte_size);
};

void pl_free(struct Pipeline*);
void pl_uniform_update(struct Pipeline*, int id, const void* data, int offset, int size);
void pl_storage_assign(struct Pipeline* p1, int storage_id, int buffer_id);
void pl_uniform_assign(struct Pipeline* p1, int uniform_id, int buffer_id);
int pl_buffer_assign(struct Pipeline* p1, int descriptor_id, int buffer_id);
void pl_use_texture(struct Pipeline* p1, void* texture);
void pl_start(struct Pipeline*);
void pl_draw(struct Pipeline* p1, int buffer_id);

struct ShaderCode {
    const char* glsl;
    const char* spir_v;
    int size;
};

struct PipelineBuilder {
    struct PipelineBuilder* (*set_bmgr)(struct PipelineBuilder*, struct BufferManager*);

    struct PipelineBuilder* (*begin_program)(struct PipelineBuilder*);
    struct PipelineBuilder* (*add_vs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*add_fs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*add_cs)(struct PipelineBuilder*, struct ShaderCode shader);
    struct PipelineBuilder* (*end_program)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_buffer)(struct PipelineBuilder*, int stride);
    struct PipelineBuilder* (*buffer_attribute)(
        struct PipelineBuilder*,
        int location,
        int channels, enum DataType data_type,
        uint64_t offset);
    struct PipelineBuilder* (*end_buffer)(struct PipelineBuilder*);

    struct PipelineBuilder* (*storage_add)(
        struct PipelineBuilder*p1,
        int binding,
        const char* name);

    struct PipelineBuilder* (*uniform_add)(
        struct PipelineBuilder*p1,
        int binding,
        const char* name);

    struct PipelineBuilder* (*begin_sampler)(struct PipelineBuilder* p1, int binding);
    struct PipelineBuilder* (*end_sampler)(struct PipelineBuilder* p1);

    struct PipelineBuilder* (*enable_depth)(struct PipelineBuilder* p1);
    struct PipelineBuilder* (*enable_blend)(struct PipelineBuilder* p1);
    struct PipelineBuilder* (*enable_cull)(struct PipelineBuilder* p1, enum CullType cull);

    struct PipelineBuilder* (*geometry)(struct PipelineBuilder* p1, enum GeometryType type);

    struct Pipeline* (*build)(struct PipelineBuilder*); // free
};
