#include <stdlib.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "mandelbrot.h"
#include "mandelbrot_vs.h"
#include "mandelbrot_fs.h"
#include "object.h"
#include "mesh.h"
#include "program.h"

struct Mandelbrot {
    struct Object base;
    struct Program* p;
    struct Mesh* m;
};

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Mandelbrot* t = (struct Mandelbrot*)obj;
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

    t->base.free = t_free;
    t->base.draw = t_draw;

    t->m = mesh1_new(vertices, 6, 0);
    t->p = prog_new();
    prog_add_vs(t->p, mandelbrot_vs);
    prog_add_fs(t->p, mandelbrot_fs);
    prog_link(t->p);
    return (struct Object*)t;
}
