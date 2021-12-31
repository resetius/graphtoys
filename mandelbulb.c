#include <stdlib.h>
#include <stdio.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "mandelbulb.h"
#include "mandelbulb_vs.h"
#include "mandelbulb_fs.h"
#include "object.h"
#include "mesh.h"
#include <render/program.h>

struct Mandelbulb {
    struct Object base;
    struct Program* p;
    struct Mesh* m;
    vec3 T;

    const char* types[10];
    int cur_type;
    int n_types;
};

static void t_left(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[0] += t->T[2]*0.01;
}

static void t_right(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[0] -= t->T[2]*0.01;
}

static void t_up(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[1] += t->T[2]*0.01;
}

static void t_down(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[1] -= t->T[2]*0.01;
}

static void t_zoom_in(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->T[2] /= 1.01;
}

static void t_zoom_out(struct Object* obj) {
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

    prog_use(t->p);
    prog_set_mat4x4(t->p, "MVP", &mvp);
    prog_set_vec3(t->p, "T", &t->T);

    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_X(rot, rot, (float)glfwGetTime());
    mat4x4_rotate_Y(rot, rot, (float)glfwGetTime());
    mat4x4_rotate_Z(rot, rot, (float)glfwGetTime());
    prog_set_mat4x4(t->p, "Rot", &rot);

    prog_set_sub_fs(t->p, t->types[t->cur_type]);

    mesh_render(t->m);
}

static void t_free(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    prog_free(t->p);
    mesh_free(t->m);
    free(t);
}

struct Object* CreateMandelbulb(struct Render* r) {
    struct Mandelbulb* t = calloc(1, sizeof(struct Mandelbulb));
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
        .zoom_out = t_zoom_out,
        .mode = t_mode
    };

    t->base = base;

    t->m = mesh1_new(vertices, 6, 0);
    t->p = rend_prog_new(r);
    t->T[0] = t->T[1] = 0.0; t->T[2] = 2.0;
    prog_add_vs(t->p, mandelbulb_vs);
    prog_add_fs(t->p, mandelbulb_fs);
    prog_link(t->p);

    t->types[0] = "next_quadratic";
    t->types[1] = "next_cubic";
    t->types[2] = "next_nine";
    t->types[3] = "next_quintic";
    t->n_types = 4;
    t->cur_type = 0;
    return (struct Object*)t;
}
