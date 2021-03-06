#include <stdlib.h>
#include <stdio.h>

#include <render/render.h>
#include <render/pipeline.h>

#include "triangle.h"
#include <lib/linmath.h>

#include <models/triangle.vert.h>
#include <models/triangle.frag.h>
#include <models/triangle.vert.spv.h>
#include <models/triangle.frag.spv.h>

typedef struct Vertex
{
    vec2 pos;
    vec3 col;
} Vertex;

static const Vertex vertices[3] =
{
    { { -0.6f, -0.4f }, { 1.f, 0.f, 0.f } },
    { {  0.6f, -0.4f }, { 0.f, 1.f, 0.f } },
    { {   0.f,  0.6f }, { 0.f, 0.f, 1.f } }
};

struct Triangle {
    struct Object base;
    struct Pipeline* pl;
    struct BufferManager* b;
    int model_buffer_id;
    int model;
    int uniform_id;
};

void tr_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Triangle* tr = (struct Triangle*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, ctx->time);
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    tr->pl->start(tr->pl);
    buffer_update(tr->b, tr->uniform_id, &mvp[0][0], 0, sizeof(mat4x4));
    tr->pl->draw(tr->pl, tr->model);
}

void tr_free(struct Object* obj) {
    struct Triangle* tr = (struct Triangle*)obj;
    tr->pl->free(tr->pl);
    free(obj);
}

struct Object* CreateTriangle(struct Render* r, struct Config* cfg) {
    struct Triangle* tr = calloc(1, sizeof(struct Triangle));
    struct Object base = {
        .draw = tr_draw,
        .free = tr_free
    };
    tr->base = base;

    struct PipelineBuilder* pl = r->pipeline(r);
    struct ShaderCode vertex_shader = {
        .glsl = models_triangle_vert,
        .spir_v = models_triangle_vert_spv,
        .size = models_triangle_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_triangle_frag,
        .spir_v = models_triangle_frag_spv,
        .size = models_triangle_frag_spv_size,
    };

    tr->b = r->buffer_manager(r);
    tr->pl = pl

        ->set_bmgr(pl, tr->b)
        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->begin_buffer(pl, sizeof(Vertex))
        ->buffer_attribute(pl, 2, 3, DATA_FLOAT, offsetof(Vertex, col)) // vCol
        ->buffer_attribute(pl, 1, 2, DATA_FLOAT, offsetof(Vertex, pos)) // vPos
        ->end_buffer(pl)

        ->build(pl);

    tr->uniform_id = buffer_create(tr->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(mat4x4));

    pl_uniform_assign(tr->pl, 0, tr->uniform_id);

    tr->model_buffer_id = buffer_create(tr->b, BUFFER_ARRAY, MEMORY_STATIC, vertices, sizeof(vertices));

    tr->model = pl_buffer_assign(tr->pl, 0, tr->model_buffer_id);

    return (struct Object*)tr;
}
