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
#include "particles_data.h"

struct Particles {
    struct Object base;
    struct Pipeline* comp;
    struct Pipeline* pl;
    struct BufferManager* b;

    int particles;

    int uniform;
    int pos;
    int new_pos;
    int accel;
    int vel;

    int pos_vao;
    int new_pos_vao;

    int model;
    float z;
    quat q;
    double ax, ay, az;
};

static int max(int a, int b) {
    return a>b?a:b;
}

// TODO: copy-paste
static void transform(struct Particles* t) {
    vec3 ox = {1,0,0};
    vec3 oy = {0,1,0};
    vec3 oz = {0,0,1};
    quat ox1 = {1,0,0,0};
    quat oy1 = {0,1,0,0};
    quat oz1 = {0,0,1,0};
    quat qx, qy, qz, q;

    quat_conj(q, t->q);

    quat_mul(ox1, ox1, t->q);
    quat_mul(ox1,  q, ox1);
    memcpy(ox, ox1, sizeof(vec3));

    quat_mul(oy1, oy1, t->q);
    quat_mul(oy1,  q, oy1);
    memcpy(oy, oy1, sizeof(vec3));

    quat_mul(oz1, oz1, t->q);
    quat_mul(oz1,  q, oz1);
    memcpy(oz, oz1, sizeof(vec3));

    quat_rotate(qx, t->ax, ox);
    quat_rotate(qy, t->ay, oy);
    quat_rotate(qz, t->az, oz);

    quat_mul(t->q, t->q, qx);
    quat_mul(t->q, t->q, qy);
    quat_mul(t->q, t->q, qz);

    t->ax = t->ay = t->az = 0;
}

static void move_left(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;

    t->az += -5*M_PI/360;
    transform(t);
}

static void move_right(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;

    t->az += 5*M_PI/360;
    transform(t);
}

static void move_up(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;

    t->ax += -5*M_PI/360;
    transform(t);
}

static void move_down(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;

    t->ax += 5*M_PI/360;
    transform(t);
}

static void zoom_in(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;
    if (mods & 1) {
        t->z /= 2;
        printf("%f\n", t->z);
    } else {
        t->ay -= 5*M_PI/360;
        transform(t);
    }
}

static void zoom_out(struct Object* obj, int mods) {
    struct Particles* t = (struct Particles*)obj;
    if (mods & 1) {
        t->z *= 2;
        printf("%f\n", t->z);
    } else {
        t->ay += 5*M_PI/360;
        transform(t);
    }
}

static void draw_(struct Object* obj, struct DrawContext* ctx) {
    struct Particles* t = (struct Particles*)obj;
    mat4x4 m, p, v, mv, mvp;
    mat4x4_identity(m);
    mat4x4_from_quat(m, t->q);

    //mat4x4_scale(m, m, 10000);
    //mat4x4_rotate_X(m, m, ctx->time);
    //mat4x4_rotate_Y(m, m, ctx->time);
    //mat4x4_rotate_Z(m, m, ctx->time);
    /*for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%f ", m[i][j]);
        }
        printf("\n");
    }
    printf("\n");*/
    //mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    //mat4x4_mul(mvp, p, m);

    //vec3 eye = {.0f, .0f, 10.f};
    vec3 eye = {.0f, .0f, t->z};
    vec3 center = {.0f, .0f, .0f};
    //vec3 up = {.0f, 1.f, .0f};
    vec3 up = {.0f, 1.f, .0f};
    mat4x4_look_at(v, eye, center, up);

    mat4x4_mul(mv, v, m);

    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 1000.f);
    mat4x4_mul(mvp, p, mv);

    //printf("particles %d\n", t->particles);
    t->comp->start_compute(t->comp, max(1, t->particles/100), 1, 1);

    //t->pl->buffer_copy(t->pl, t->pos, t->new_pos);

    t->pl->start(t->pl);
    buffer_update(t->b, t->uniform, &mvp[0][0], 0, sizeof(mat4x4));

    //t->pl->draw(t->pl, t->new_pos_vao);
    t->pl->draw(t->pl, t->pos_vao);

    int tt = t->pos_vao; t->pos_vao = t->new_pos_vao; t->new_pos_vao = tt;
    t->comp->storage_swap(t->comp, 0, 3);
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    t->comp->free(t->comp);
    t->b->free(t->b);
    free(obj);
}

struct Object* CreateParticles(struct Render* r, struct Config* cfg) {
    struct Particles* t = calloc(1, sizeof(struct Particles));
    struct Object base = {
        .move_left = move_left,
        .move_right = move_right,
        .move_up = move_up,
        .move_down = move_down,

        .zoom_in = zoom_in,
        .zoom_out = zoom_out,
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

    t->b = r->buffer_manager(r);

    t->comp = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_shader)
        ->end_program(pl)

        ->storage_add(pl, 0, "Pos")
        ->storage_add(pl, 1, "Vel")
        ->storage_add(pl, 2, "Tmp")
        ->storage_add(pl, 3, "NewPos")

        ->build(pl);

    pl = r->pipeline(r);

    t->pl = pl
        ->set_bmgr(pl, t->b)

        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->begin_buffer(pl, 4*sizeof(float))
        ->buffer_attribute(pl, 1, 4, DATA_FLOAT, 0)
        ->end_buffer(pl)

        ->geometry(pl, GEOM_POINTS)

        ->build(pl);

    quat_identity(t->q);

    struct ParticlesData data;
    particles_data_init(&data, cfg);

    t->z = 1.0f;
    t->particles = data.n_particles;

    int size = t->particles*4*sizeof(float);

    t->uniform = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(mat4x4));

    t->pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, data.coords, size);
    t->vel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, data.vels, size);
    t->accel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, data.accel, size);
    t->new_pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, data.coords, size);

    t->pos_vao = t->pl->buffer_assign(t->pl, 0, t->pos);
    t->new_pos_vao = t->pl->buffer_assign(t->pl, 0, t->new_pos);

    t->pl->uniform_assign(t->pl, 0, t->uniform);

    t->comp->storage_assign(t->comp, 0, t->pos);
    t->comp->storage_assign(t->comp, 1, t->vel);
    t->comp->storage_assign(t->comp, 2, t->accel);
    t->comp->storage_assign(t->comp, 3, t->new_pos);

    return (struct Object*)t;
}
