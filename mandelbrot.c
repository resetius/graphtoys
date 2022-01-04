#include <stdlib.h>
#include <stdio.h>

#include <render/pipeline.h>

#include "mandelbrot.h"
#include "mandelbrot_vs.vert.h"
#include "mandelbrot_fs.frag.h"
#include "mandelbrot_vs.vert.spv.h"
#include "mandelbrot_fs.frag.spv.h"

#include "object.h"
#include "linmath.h"

struct Vertex {
    vec3 pos;
};

struct UniformBlock {
    mat4x4 mvp;
    vec3 T;
};

struct Mandelbrot {
    struct Object base;
    struct UniformBlock uniform;
    struct Pipeline* pl;
    vec3 T;
};

static void t_left(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[0] += t->T[2]*0.01;
}

static void t_right(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[0] -= t->T[2]*0.01;
}

static void t_up(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[1] += t->T[2]*0.01;
}

static void t_down(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[1] -= t->T[2]*0.01;
}

static void t_zoom_in(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[2] /= 1.01;
}

static void t_zoom_out(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->T[2] *= 1.01;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
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

    memcpy(t->uniform.mvp, mvp, sizeof(mvp));
    memcpy(t->uniform.T, t->T, sizeof(t->T));

    t->pl->uniform_update(t->pl, 0, &t->uniform, 0, sizeof(t->uniform));
    t->pl->run(t->pl);
}

static void t_free(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    t->pl->free(t->pl);
    free(t);
}

struct Object* CreateMandelbrot(struct Render* r) {
    struct Mandelbrot* t = calloc(1, sizeof(struct Mandelbrot));
    struct Vertex vertices[] = {
        {{-1.0f, 1.0f,0.0f}},
        {{-1.0f,-1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},

        {{-1.0f, 1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},
        {{ 1.0f, 1.0f,0.0f}}
    };
    int nvertices = 6;

    struct Object base = {
        .draw = t_draw,
        .free = t_free,
        .move_left = t_left,
        .move_right = t_right,
        .move_up = t_up,
        .move_down = t_down,
        .zoom_in = t_zoom_in,
        .zoom_out = t_zoom_out
    };

    struct ShaderCode vertex_shader = {
        .glsl = mandelbrot_vs_vert,
        .spir_v = mandelbrot_vs_vert_spv,
        .size = mandelbrot_vs_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = mandelbrot_fs_frag,
        .spir_v = mandelbrot_fs_frag_spv,
        .size = mandelbrot_fs_frag_spv_size,
    };

    t->base = base;

    struct PipelineBuilder* pl = r->pipeline(r);
    t->pl = pl->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->begin_uniform(pl, 0, "MatrixBlock", sizeof(struct UniformBlock))
        ->end_uniform(pl)

        ->begin_buffer(pl, nvertices, sizeof(struct Vertex))
        ->buffer_data(pl, vertices, nvertices*sizeof(struct Vertex)) //TODO: size dont need
        ->buffer_attribute(pl, 1, 3, 4, offsetof(struct Vertex, pos))
        ->end_buffer(pl)

        ->build(pl);

    t->T[0] = t->T[1] = 0.0; t->T[2] = 2.0;

    return (struct Object*)t;
}
