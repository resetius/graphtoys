#include <stdlib.h>
#include <stdio.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <lib/linmath.h>
#include <lib/config.h>

#include <models/particles2.vert.h>
#include <models/particles2.frag.h>
#include <models/particles2.vert.spv.h>
#include <models/particles2.frag.spv.h>

#include "particles2.h"
#include "particles_data.h"

struct UniformBlock {
    mat4x4 mvp;
    int32_t particles;
};

struct Particles {
    struct Object base;
    struct Pipeline* pl;
    struct BufferManager* b;

    int particles;

    int pos;
    int new_pos;
    int accel;
    int vel;

    int uniform;

    int indices;
    int indices_vao;

    int model;
    float z;
    quat q;
    double ax, ay, az;

    int dirty;
};

const int clear_frames = 5;

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
    t->dirty = clear_frames;
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
    t->dirty = clear_frames;
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
    t->dirty = clear_frames;
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

    t->pl->start(t->pl);

    if (t->dirty) {
        struct UniformBlock block;
        block.particles = t->particles;
        memcpy(block.mvp, mvp, sizeof(mvp));
        buffer_update(t->b, t->uniform, &block, 0, sizeof(block));
        t->dirty--;
    }

    t->pl->draw(t->pl, t->indices_vao);

    t->pl->storage_swap(t->pl, 1, 4);
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    t->b->free(t->b);
    free(obj);
}

struct Object* CreateParticles2(struct Render* r, struct Config* cfg) {
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
        .glsl = models_particles2_vert,
        .spir_v = models_particles2_vert_spv,
        .size = models_particles2_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_particles2_frag,
        .spir_v = models_particles2_frag_spv,
        .size = models_particles2_frag_spv_size,
    };

    t->b = r->buffer_manager(r);

    pl = r->pipeline(r);

    t->pl = pl
        ->set_bmgr(pl, t->b)

        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->storage_add(pl, 1, "Pos")
        ->storage_add(pl, 2, "Vel")
        ->storage_add(pl, 3, "Tmp")
        ->storage_add(pl, 4, "NewPos")

        ->begin_buffer(pl, 4)
        ->buffer_attribute(pl, 1, 1, DATA_INT, 0)
        ->end_buffer(pl)

        ->geometry(pl, GEOM_POINTS)

        ->build(pl);

    quat_identity(t->q);

    struct ParticlesData data;
    particles_data_init(&data, cfg);

    t->z = 1.0f;
    t->particles = data.n_particles;

    int size = t->particles*4*sizeof(float);

    t->dirty = clear_frames;
    t->indices = t->b->create(t->b, BUFFER_ARRAY, MEMORY_STATIC, data.indices, t->particles*sizeof(int));

    t->uniform = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));

    t->pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.coords, size);
    t->vel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.vels, size);
    t->accel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.accel, size);
    t->new_pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.coords, size);

    t->indices_vao = t->pl->buffer_assign(t->pl, 0, t->indices);

    t->pl->uniform_assign(t->pl, 0, t->uniform);

    t->pl->storage_assign(t->pl, 1, t->pos);
    t->pl->storage_assign(t->pl, 2, t->vel);
    t->pl->storage_assign(t->pl, 3, t->accel);
    t->pl->storage_assign(t->pl, 4, t->new_pos);


    particles_data_destroy(&data);
    return (struct Object*)t;
}
