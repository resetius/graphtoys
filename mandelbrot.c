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

struct Object* CreateMandlebrot() {
    struct Mandelbrot* t = calloc(1, sizeof(struct Mandelbrot));
    t->base.free = t_free;
    t->base.draw = t_draw;

    t->p = prog_new();
    return (struct Object*)t;
}
