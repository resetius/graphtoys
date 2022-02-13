#include <stdlib.h>
#include <stdio.h>

#include "mandelbulb.h"
#include <models/mandelbulb.vert.h>
#include <models/mandelbulb.frag.h>
#include <models/mandelbulb.vert.spv.h>
#include <models/mandelbulb.frag.spv.h>
#include <lib/object.h>
#include <lib/linmath.h>

#include <render/pipeline.h>

struct UniformBlock {
    mat4x4 mvp;
    mat4x4 rot;
    mat4x4 inv_rot;
    vec4 T;
    int next;
};

struct Mandelbulb {
    struct Object base;
    vec3 T;
    struct BufferManager* b;
    int uniform_id;

    int cur_type;
    int n_types;

    struct UniformBlock uniform;
    struct Pipeline* pl;

    int model;
};

static void t_left(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[0] += t->T[2]*0.01;
}

static void t_right(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[0] -= t->T[2]*0.01;
}

static void t_up(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[1] += t->T[2]*0.01;
}

static void t_down(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[1] -= t->T[2]*0.01;
}

static void t_zoom_in(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[2] /= 1.01;
}

static void t_zoom_out(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[2] *= 1.01;
}

static void t_mode(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->cur_type = (t->cur_type + 1) % t->n_types;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    //vec3 T = {0.0f, 0.0f, 2.0f};
    mat4x4 m, v, mv, p, mvp;
    mat4x4_identity(m);
    //mat4x4_rotate_X(m, m, (float)glfwGetTime());
    //mat4x4_rotate_Y(m, m, (float)glfwGetTime());
    //mat4x4_rotate_Z(m, m, (float)glfwGetTime());

    //mat4x4_rotate_Z(m, m, (float)glfwGetTime());
    //mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    //mat4x4_mul(mvp, p, m);
    vec3 eye = {.0f, .0f, -1.f};
    vec3 center = {.0f, .0f, .0f};
    vec3 up = {.0f, 1.f, .0f};
    mat4x4_look_at(v, eye, center, up);
    mat4x4_mul(mv, v, m);
    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 100.f);
    mat4x4_mul(mvp, p, mv);

    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_X(rot, rot, 0.5*ctx->time);
    mat4x4_rotate_Y(rot, rot, 0.5*ctx->time);
    mat4x4_rotate_Z(rot, rot, 0.5*ctx->time);

    //mat3x3 norm;
    //mat3x3_from_mat4x4(norm, rot);

    memcpy(t->uniform.mvp, mvp, sizeof(mvp));
    memcpy(t->uniform.rot, rot, sizeof(rot));
    mat4x4_invert(t->uniform.inv_rot, rot);
    memcpy(t->uniform.T, t->T, sizeof(t->T));
    memcpy(&t->uniform.next, &t->cur_type, 4);

    t->pl->start(t->pl);
    buffer_update(t->b, t->uniform_id, &t->uniform, 0, sizeof(t->uniform));
    t->pl->draw(t->pl, t->model);
}

static void t_free(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->pl->free(t->pl);
    t->b->free(t->b);
    free(t);
}

struct Vertex {
    vec3 pos;
};

struct Object* CreateMandelbulb(struct Render* r, struct Config* cfg) {
    struct Mandelbulb* t = calloc(1, sizeof(struct Mandelbulb));
    struct Vertex vertices[] = {
        {{ 1.0f,-1.0f,0.0f}},
        {{-1.0f,-1.0f,0.0f}},
        {{-1.0f, 1.0f,0.0f}},

        {{ 1.0f, 1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},
        {{-1.0f, 1.0f,0.0f}}
    };
    int n_vertices = 6;

    struct Object base = {
        .draw = t_draw,
        .free = t_free,
        .move_left = t_left,
        .move_right = t_right,
        .move_up = t_up,
        .move_down = t_down,
        .zoom_in = t_zoom_in,
        .zoom_out = t_zoom_out,
        .mode = t_mode
    };

    struct ShaderCode vertex_shader = {
        .glsl =models_mandelbulb_vert,
        .spir_v =models_mandelbulb_vert_spv,
        .size = models_mandelbulb_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_mandelbulb_frag,
        .spir_v = models_mandelbulb_frag_spv,
        .size = models_mandelbulb_frag_spv_size,
    };

    t->b = r->buffer_manager(r);
    t->base = base;
    t->T[0] = t->T[1] = 0.0; t->T[2] = 2.0;

    struct PipelineBuilder* p = r->pipeline(r);
    t->pl = p
        ->set_bmgr(p, t->b)

        ->begin_program(p)
        ->add_vs(p, vertex_shader)
        ->add_fs(p, fragment_shader)
        ->end_program(p)

        ->uniform_add(p, 0, "MatrixBlock")

        ->begin_buffer(p, sizeof(struct Vertex))
        ->buffer_attribute(p, 1, 3, DATA_FLOAT, offsetof(struct Vertex, pos))
        ->end_buffer(p)

        ->build(p);

    t->uniform_id = buffer_create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));
    pl_uniform_assign(t->pl, 0, t->uniform_id);

    int model_buffer_id = buffer_create(t->b, BUFFER_ARRAY, MEMORY_STATIC, vertices, n_vertices*sizeof(struct Vertex));
    t->model = pl_buffer_assign(t->pl, 0, model_buffer_id);

    t->n_types = 4;
    t->cur_type = 0;
    return (struct Object*)t;
}
