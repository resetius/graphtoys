#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <lib/linmath.h>

#include <models/particles.vert.h>
#include <models/particles.frag.h>
#include <models/particles.comp.h>
#include <models/particles.vert.spv.h>
#include <models/particles.frag.spv.h>
#include <models/particles.comp.spv.h>

#include "particles.h"

struct Particles {
    struct Object base;
    struct Pipeline* pl;
    int particles;
    int pos;
    int vel;
    int model;
};

static void draw_(struct Object* obj, struct DrawContext* ctx) {
    struct Particles* t = (struct Particles*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    //mat4x4_rotate_X(m, m, ctx->time);
    //mat4x4_rotate_Y(m, m, ctx->time);
    //mat4x4_rotate_Z(m, m, ctx->time);
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    t->pl->start_compute_part(t->pl, 0, t->particles/1000, 1, 1);
    t->pl->wait_part(t->pl, 0);

    t->pl->start_part(t->pl, 1);
    t->pl->uniform_update(t->pl, 0, &mvp[0][0], 0, sizeof(mat4x4));
    t->pl->draw(t->pl, t->pos);
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    free(obj);
}

struct Object* CreateParticles(struct Render* r) {
    struct Particles* t = calloc(1, sizeof(struct Particles));
    struct Object base = {
        .draw = draw_,
        .free = free_
    };
    t->base = base;

    struct PipelineBuilder* pl = r->pipeline(r);
    struct ShaderCode vertex_shader = {
        .glsl = models_particles_vert,
        .spir_v = models_particles_vert_spv,
        .size = models_particles_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_particles_frag,
        .spir_v = models_particles_frag_spv,
        .size = models_particles_frag_spv_size,
    };
    struct ShaderCode compute_shader = {
        .glsl = models_particles_comp,
        .spir_v = models_particles_comp_spv,
        .size = models_particles_comp_spv_size,
    };
    t->pl = pl
        ->begin_program(pl)
        ->add_cs(pl, compute_shader)
        ->end_program(pl)

        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->begin_uniform(pl, 0, "MatrixBlock", sizeof(mat4x4))
        ->end_uniform(pl)

        ->begin_buffer(pl, 4*sizeof(float))
        ->buffer_attribute(pl, 1, 4, 4, 0)
        ->end_buffer(pl)

        ->geometry(pl, GEOM_POINTS)

        ->build(pl);

    int n_x = 16, n_y = 16, n_z = 16;
    int n_particles = n_x*n_y*n_z;
    int size = n_particles*4*sizeof(float);
    float* coords = malloc(size);
    float* vels = calloc(1, size);
    int i,j,k,n=0;
    float dx = 4.0f/(n_x-1);
    float dy = 4.0f/(n_y-1);
    float dz = 4.0f/(n_z-1);

    srand(time(NULL));

    for (i = 0; i < n_x; i++) {
        for (j = 0; j < n_y; j++) {
            for (k = 0; k < n_z; k++) {
                coords[n++] = dx * i - 2.0f;
                coords[n++] = dy * j - 2.0f;
                coords[n++] = dz * k - 2.0f;
                coords[n++] = 0.2 + 1.5*(double)rand() / (double)RAND_MAX;
            }
        }
    }

    t->particles = n;

    t->pos = pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, 1, 0, coords, size);
    t->vel = pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, 2, -1, vels, size);
    // tmp
    //pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, 3, -1, NULL, size);

    free(coords);
    free(vels);
    return (struct Object*)t;
}
