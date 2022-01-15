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
    int new_pos;
    int vel;
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

    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 100.f);
    mat4x4_mul(mvp, p, mv);

    //printf("particles %d\n", t->particles);
    t->pl->start_compute_part(t->pl, 0, max(1, t->particles/100), 1, 1);
    t->pl->wait_part(t->pl, 0);

    //t->pl->buffer_copy(t->pl, t->pos, t->new_pos);

    t->pl->start_part(t->pl, 1);
    t->pl->uniform_update(t->pl, 0, &mvp[0][0], 0, sizeof(mat4x4));

    t->pl->draw(t->pl, t->new_pos);

    int tt = t->pos; t->pos = t->new_pos; t->new_pos = tt;
    t->pl->buffer_swap(t->pl, t->pos, t->new_pos);
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    free(obj);
}

struct body
{
    double x[3];
    double v[3];
    double mass;
    int fixed;
    char name[100];
};

struct Object* CreateParticles(struct Render* r) {
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

    int n_x = 32, n_y = 32, n_z = 32;
    int n_particles = n_x*n_y*n_z;
    int size = n_particles*4*sizeof(float);
    float* coords = malloc(size);
    float* vels = calloc(1, size);
    float* accel = calloc(1, size);
    int i,j,k,n=0;
    float dx = 4.0f/(n_x-1);
    float dy = 4.0f/(n_y-1);
    float dz = 4.0f/(n_z-1);

    quat_identity(t->q);

    srand(time(NULL));

    const double G = 2.92e-6;
/*
    struct body body[] = {
        {{0, 0, 0}, {0, 0, 0}, 10000, 1, "Sun"},
        {{0, 1, 0}, {-1, 0, 0}, 1, 0, "Earth"},
    };
    const int nbodies = 2;
*/
    struct body body[] = {
        {{0, 0, 0}, {0, 0, 0}, 333333, 1, "Sun"},
        {{0, 0.39, 0}, {1.58, 0, 0}, 0.038, 0, "Mercury"},
        {{0, 0.72, 0}, {1.17, 0, 0}, 0.82, 0, "Venus"},
        {{0, 1, 0}, {1, 0, 0}, 1, 0, "Earth"},
        {{0, 1.00256, 0}, {1.03, 0, 0}, 0.012, 0, "Moon"},
        {{0, 1.51, 0}, {0.8, 0, 0}, 0.1, 0, "Mars"},
        {{0, 5.2, 0}, {0.43, 0, 0}, 317, 0, "Jupiter"},
        {{0, 9.3, 0}, {0.32, 0, 0}, 95, 0, "Saturn"},
        {{0, 19.3, 0}, {0.23, 0, 0}, 14.5, 0, "Uranus"},
        {{0, 30, 0}, {0.18, 0, 0}, 16.7, 0, "Neptune"}};

    const int nbodies = 10;

/*
    for (i = 0; i < nbodies; i++) {
        coords[n] = body[i].x[0];
        coords[n+1] = body[i].x[1];
        coords[n+2] = body[i].x[2];
        coords[n+3] = G * body[i].mass;

        vels[n] = body[i].v[0];
        vels[n+1] = body[i].v[1];
        vels[n+2] = body[i].v[2];
        vels[n+3] = 0;

        n += 4;
    }
*/

    for (i = 0; i < n_x; i++) {
        for (j = 0; j < n_y; j++) {
            for (k = 0; k < n_z; k++) {
                coords[n] = dx * i - 2.0f + 0.5 * (double)rand() / (double)RAND_MAX;
                coords[n+1] = dy * j - 2.0f + 0.5 * (double)rand() / (double)RAND_MAX;
                coords[n+2] = dz * k - 2.0f + 0.5 * (double)rand() / (double)RAND_MAX;
                coords[n+3] = 0.2 + 1.5*(double)rand() / (double)RAND_MAX;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);
                double V = sqrt(1000)/sqrt(R); // sqrt(100/R);

                vels[n] = V*coords[n+1];
                vels[n+1] = -V*coords[n];
                vels[n+2] = 0;

                //vels[n] = rand() / (double) RAND_MAX;
                //vels[n+1] = rand() / (double) RAND_MAX;
                //vels[n+2] = rand() / (double) RAND_MAX;
                vels[n+3] = 0;

                n += 4;
            }
        }
    }


    t->z = 1.0f;
    t->particles = n/4;
/*
    for (i = 0; i < t->particles; i++) {
        printf("%f %f %f %f, %f %f %f %f\n",
               coords[i*4+0],coords[i*4+1],coords[i*4+2],coords[i*4+3],
               vels[i*4+0],vels[i*4+1],vels[i*4+2],vels[i*4+3]
            );
    }
*/
    size = t->particles*4*sizeof(float);

    t->pos = pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, 1, 0, coords, size);
    t->vel = pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, 2, -1, vels, size);
    pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, 3, -1, accel, size);

    t->new_pos = pl_buffer_storage_create(t->pl, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, 4, 0, coords, size);

    free(coords);
    free(vels);
    free(accel);
    return (struct Object*)t;
}
