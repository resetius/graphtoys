#include <stdlib.h>

#include "mandelbrot.h"
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
    prog_use(t->p);
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
        {{0.0f,0.0f,0.0f}},
        {{1.0f,0.0f,0.0f}},
        {{1.0f,1.0f,0.0f}},
        {{0.0f,1.0f,0.0f}}
    };

    t->base.free = t_free;
    t->base.draw = t_draw;

    t->m = mesh1_new(vertices, 4, 0);
    t->p = prog_new();
    return (struct Object*)t;
}
