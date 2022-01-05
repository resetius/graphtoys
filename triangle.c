#include <stdlib.h>
#include <stdio.h>

#include <render/render.h>
#include <render/pipeline.h>

#include "triangle.h"
#include "linmath.h"

#include "triangle_vertex_shader.vert.h"
#include "triangle_fragment_shader.frag.h"
#include "triangle_vertex_shader.vert.spv.h"
#include "triangle_fragment_shader.frag.spv.h"

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
};

void tr_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Triangle* tr = (struct Triangle*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, ctx->time);
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    tr->pl->uniform_update(tr->pl, 0, &mvp[0][0], 0, sizeof(mat4x4));
    tr->pl->run(tr->pl);
}

void tr_free(struct Object* obj) {
    struct Triangle* tr = (struct Triangle*)obj;
    tr->pl->free(tr->pl);
    free(obj);
}

struct Object* CreateTriangle(struct Render* r) {
    struct Triangle* tr = calloc(1, sizeof(struct Triangle));
    struct Object base = {
        .draw = tr_draw,
        .free = tr_free
    };
    tr->base = base;

    struct PipelineBuilder* pl = r->pipeline(r);
    struct ShaderCode vertex_shader = {
        .glsl = triangle_vertex_shader_vert,
        .spir_v = triangle_vertex_shader_vert_spv,
        .size = triangle_vertex_shader_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = triangle_fragment_shader_frag,
        .spir_v = triangle_fragment_shader_frag_spv,
        .size = triangle_fragment_shader_frag_spv_size,
    };
    tr->pl = pl->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->begin_uniform(pl, 0, "MatrixBlock", sizeof(mat4x4))
        ->end_uniform(pl)

        ->begin_buffer(pl, sizeof(Vertex))
        ->buffer_data(pl, (void*)vertices, sizeof(vertices))
        ->buffer_attribute(pl, 2, 3, 4, offsetof(Vertex, col)) // vCol
        ->buffer_attribute(pl, 1, 2, 4, offsetof(Vertex, pos)) // vPos
        ->end_buffer(pl)

        ->build(pl);

    return (struct Object*)tr;
}
