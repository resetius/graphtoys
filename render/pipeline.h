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

    // deprecated
    void (*buffer_update)(
        struct Pipeline*,
        int buffer_od,
        const void* data,
        int offset,
        int size);

    // deprecated
    void (*buffer_copy)(struct Pipeline* p1,
                        int dst, int src);
    void (*storage_swap)(struct Pipeline* p1,
                         int dst, int src);

    // deprecated
    int (*buffer_create)(
        struct Pipeline* p1,
        enum BufferType type,
        enum BufferMemoryType mem_type,
        int descriptor,
        const void* data,
        int size); // -> buffer id

    void (*use_texture)(struct Pipeline* p1, void* texture);

    void (*start_compute)(struct Pipeline* p1, int sx, int sy, int sz);

    void (*start)(struct Pipeline* p1);
    void (*draw)(struct Pipeline* p1, int buffer_id);
};

void pl_free(struct Pipeline*);
void pl_uniform_update(struct Pipeline*, int id, const void* data, int offset, int size);
void pl_buffer_update(struct Pipeline*, int od, const void* data, int offset, int size);
int pl_buffer_create(struct Pipeline*, enum BufferType type, enum BufferMemoryType mtype,
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

    // deprecated
    struct PipelineBuilder* (*begin_uniform)(
        struct PipelineBuilder*,
        int binding,
        const char* blockName,
        int size);
    // deprecated
    struct PipelineBuilder* (*end_uniform)(struct PipelineBuilder*);

    struct PipelineBuilder* (*begin_sampler)(struct PipelineBuilder* p1, int binding);
    struct PipelineBuilder* (*end_sampler)(struct PipelineBuilder* p1);

    struct PipelineBuilder* (*enable_depth)(struct PipelineBuilder* p1);
    struct PipelineBuilder* (*enable_blend)(struct PipelineBuilder* p1);

    struct PipelineBuilder* (*geometry)(struct PipelineBuilder* p1, enum GeometryType type);

    struct Pipeline* (*build)(struct PipelineBuilder*); // free
};
