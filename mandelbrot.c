#include <stdlib.h>
#include <stdio.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "mandelbrot.h"
#include "mandelbrot_vs.h"
#include "mandelbrot_fs.h"
#include "object.h"
#include "mesh.h"
#include <render/program.h>

struct Mandelbrot {
    struct Object base;
    struct Program* p;
    struct Mesh* m;
    vec3 T;
    float sx;
    float sy;
    float sz;
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

    prog_use(t->p);
    prog_set_mat4x4(t->p, "MVP", &mvp);
    prog_set_vec3(t->p, "T", &t->T);
/*
    t->T[0] += t->sx*0.01f;
    if (t->T[0] > 1.0 || t->T[0] < -1.0) {
        t->sx = -t->sx;
    }
    t->T[1] += t->sx*0.01f;
    if (t->T[1] > 1.0 || t->T[1] < -1.0) {
        t->sy = -t->sy;
    }
    t->T[2] *= t->sz;
    if (t->T[2] > 4.0 || t->T[2] < 0.1) {
        t->sz = 1./t->sz;
    }
*/
    mesh_render(t->m);
}

static void t_free(struct Object* obj) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
    prog_free(t->p);
    mesh_free(t->m);
    free(t);
}

struct Object* CreateMandelbrot() {
    struct Mandelbrot* t = calloc(1, sizeof(struct Mandelbrot));
    struct Vertex1 vertices[] = {
        {{-1.0f, 1.0f,0.0f}},
        {{-1.0f,-1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},

        {{-1.0f, 1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},
        {{ 1.0f, 1.0f,0.0f}}
    };

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

    t->base = base;

    t->m = mesh1_new(vertices, 6, 0);
    t->p = prog_new();
    t->T[0] = t->T[1] = 0.0; t->T[2] = 2.0;
    t->sx = 1.0;
    t->sy = -1.0;
    t->sz = 1.01;
    prog_add_vs(t->p, mandelbrot_vs);
    prog_add_fs(t->p, mandelbrot_fs);
    prog_link(t->p);
    return (struct Object*)t;
}
