#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <lib/linmath.h>

#include <models/particles3.vert.h>
#include <models/particles2.frag.h>
#include <models/particles3_pm.comp.h>
#include <models/particles3.vert.spv.h>
#include <models/particles2.frag.spv.h>
#include <models/particles3_pm.comp.spv.h>

#include <lib/verify.h>
#include <lib/config.h>
#include "particles3.h"
#include "particles_data.h"

struct CompSettings {
    vec4 origin;
    int particles;
    int nn;  // grid nn x nn xnn
    int n;   // log2(n)
    float h; // l/h
    float l; // length of cube edge
};

struct VertBlock {
    mat4x4 mvp;
    vec4 origin;
    float h;
    int nn;
};

struct Particles {
    struct Object base;
    struct Pipeline* comp;
    struct Pipeline* pl;
    struct BufferManager* b;

    int particles; // number of particles

    // compute
    struct CompSettings comp_set;

    int density_index;
    float* density;
    int psi_index;
    float* psi;
    int e_index;
    int fft_table_index;
    int work_index;
    int comp_settings;
    //

    struct VertBlock vert;
    int indices;
    int indices_vao;
    int uniform;
    int pos;
    int new_pos;
    int accel;
    int vel;
    float* pos_data;

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

static void cic3(float M[2][2][2], float x, float y, float  z, int* x0, int* y0, int* z0, float h) {
    int j = floor(x / h);
    int k = floor(y / h);
    int i = floor(z / h);
    *x0 = j;
    *y0 = k;
    *z0 = i;

    x = (x-j*h)/h;
    y = (y-k*h)/h;
    z = (z-i*h)/h;

    verify(0 <= x && x <= 1);
    verify(0 <= y && y <= 1);
    verify(0 <= z && z <= 1);

    M[0][0][0] = (1-z)*(1-y)*(1-x);
    M[0][0][1] = (1-z)*(1-y)*(x);
    M[0][1][0] = (1-z)*(y)*(1-x);
    M[0][1][1] = (1-z)*(y)*(x);

    M[1][0][0] = (z)*(1-y)*(1-x);
    M[1][0][1] = (z)*(1-y)*(x);
    M[1][1][0] = (z)*(y)*(1-x);
    M[1][1][1] = (z)*(y)*(x);
}

static void distribute(int nn, float G, float* density, float* coord, int nparticles, float h, float origin[3]) {
    float M[2][2][2] = {0};
    memset(density, 0, nn*nn*nn*sizeof(float));
#define off(i,k,j) ((i)%nn)*nn*nn+((k)%nn)*nn+((j)%nn)
    for (int index = 0; index < nparticles; index++) {
        float* c = &coord[index*4];
        float x = c[0]; float y = c[1]; float z = c[2];
        float mass = c[3];
        int i0, k0, j0;

        cic3(M, x-origin[0], y-origin[1], z-origin[2], &i0, &k0, &j0, h);

        for (int i = 0; i < 2; i++) {
            for (int k = 0; k < 2; k++) {
                for (int j = 0; j < 2; j++) {
                    density[off(i+i0,k+k0,j+j0)] += 4*M_PI*G*mass*M[i][k][j]/h/h/h;
                }
            }
        }
    }

#undef off
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

    int nn = t->vert.nn;
/*
    t->b->read(t->b, t->pos, t->pos_data, 0, 4*t->particles*sizeof(float));
    for (int i = 0; i < t->particles; i++) {
        float* p = &t->pos_data[i*4];
        printf("%d: %e %e %e\n", i, p[0], p[1], p[2]);
        for (int m = 0; m < 3; m++) {
            verify (p[m] >= t->vert.origin[m]);
            verify (p[m] < t->vert.origin[m] + t->comp_set.l);
        }
    }
*/
//    distribute(nn, 1, t->density, t->pos_data, t->particles, t->vert.h, t->vert.origin);
//    buffer_update(t->b, t->density_index, t->density, 0, nn*nn*nn*sizeof(float));

    //printf("particles %d\n", t->particles);
    buffer_update(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set));
    t->comp->start_compute(t->comp, 1, 1, 1);
//    int nn = t->comp_set.nn;
//    t->b->read(t->b, t->psi_index, t->psi, 0, nn*nn*nn*sizeof(float));

    t->pl->start(t->pl);
    memcpy(t->vert.mvp, &mvp[0][0], sizeof(mat4x4));
    buffer_update(t->b, t->uniform, &t->vert, 0, sizeof(t->vert));

    t->pl->draw(t->pl, t->indices_vao);

//    t->pl->storage_swap(t->pl, 1, 2);
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    t->comp->free(t->comp);
    t->b->free(t->b);
    free(t->psi);
    free(t->pos_data);
    free(t->density);
    free(obj);
}

struct Object* CreateParticles3(struct Render* r, struct Config* cfg) {
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
        .glsl = models_particles3_vert,
        .spir_v = models_particles3_vert_spv,
        .size = models_particles3_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_particles2_frag,
        .spir_v = models_particles2_frag_spv,
        .size = models_particles2_frag_spv_size,
    };
    struct ShaderCode compute_shader = {
        .glsl = models_particles3_pm_comp,
        .spir_v = models_particles3_pm_comp_spv,
        .size = models_particles3_pm_comp_spv_size,
    };

    t->b = r->buffer_manager(r);

    printf("Build comp shader\n");
    t->comp = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")

        ->storage_add(pl, 1, "FFTBuffer")
        ->storage_add(pl, 2, "WorkBuffer")
        ->storage_add(pl, 3, "DensityBuffer")
        ->storage_add(pl, 4, "PotentialBuffer")
        ->storage_add(pl, 5, "EBuffer")
        ->storage_add(pl, 6, "PosBuffer")

        ->build(pl);
    printf("Done\n");

    pl = r->pipeline(r);

    t->pl = pl
        ->set_bmgr(pl, t->b)

        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->storage_add(pl, 1, "PosBuffer")
        ->storage_add(pl, 2, "NewPosBuffer")
        ->storage_add(pl, 3, "VelBuffer")
        ->storage_add(pl, 4, "AccelBuffer")
        ->storage_add(pl, 5, "EBuffer")

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

    float x0 = cfg_getf_def(cfg, "x0", -1000);
    float y0 = cfg_getf_def(cfg, "y0", -1000);
    float z0 = cfg_getf_def(cfg, "z0", -1000);
    float l = cfg_getf_def(cfg, "l", 2000);

    float origin[] = {x0, y0, z0};
    int nn = 32; // TODO: parameters
    float h = l/nn;
    t->comp_set.nn = nn;
    t->comp_set.n = 31-__builtin_clz(nn);
    t->comp_set.h = h;
    t->comp_set.l = l;
    t->comp_set.particles = t->particles;

    memcpy(t->vert.origin, origin, 3*sizeof(float));
    memcpy(t->comp_set.origin, origin, 3*sizeof(float));

    t->vert.h = h;
    t->vert.nn = nn;

    t->density = NULL;
    t->density = malloc(nn*nn*nn*sizeof(float));
    distribute(nn, 1, t->density, data.coords, t->particles, h, origin);
    t->density_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, t->density,
                                    nn*nn*nn*sizeof(float));
    t->psi = malloc(nn*nn*nn*sizeof(float));
    t->psi_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_READ, NULL,
                                nn*nn*nn*sizeof(float));

    t->e_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_READ, NULL,
                              4*nn*nn*nn*sizeof(float));

    int fft_table_size = 3*2*nn*sizeof(float);
    float* fft_table = malloc(3*2*nn*sizeof(float));
    int m = 0, m1;
    for (; m < 2*nn; m++) {
        fft_table[m] = cos(m * M_PI/nn);
    }
    for (m1 = 0; m1 < 2*nn; m++, m1++) {
        fft_table[m] = sin(m1 * M_PI/nn);
    }
    for (m1 = 0; m1 < nn; m++, m1++) {
        fft_table[m] = 4./h/h*sin(m1*M_PI/nn)*sin(m1*M_PI/nn);
    }

    t->fft_table_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, fft_table, fft_table_size);
    free(fft_table);
    t->work_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL, 2*nn*nn*nn*sizeof(float));
    t->comp_settings = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct CompSettings));

    t->indices = t->b->create(t->b, BUFFER_ARRAY, MEMORY_STATIC, data.indices, t->particles*sizeof(int));

    t->uniform = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct VertBlock));

    t->pos_data = malloc(size);

    t->pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_READ, data.coords, size);
    t->new_pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_READ, data.coords, size);
    t->vel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.vels, size);
    t->accel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.accel, size);

    t->indices_vao = t->pl->buffer_assign(t->pl, 0, t->indices);

    t->pl->uniform_assign(t->pl, 0, t->uniform);
    t->pl->storage_assign(t->pl, 1, t->pos);
    t->pl->storage_assign(t->pl, 2, t->new_pos);
    t->pl->storage_assign(t->pl, 3, t->vel);
    t->pl->storage_assign(t->pl, 4, t->accel);
    t->pl->storage_assign(t->pl, 5, t->e_index);

    t->comp->uniform_assign(t->comp, 0, t->comp_settings);
    t->comp->storage_assign(t->comp, 1, t->fft_table_index);
    t->comp->storage_assign(t->comp, 2, t->work_index);
    t->comp->storage_assign(t->comp, 3, t->density_index);
    t->comp->storage_assign(t->comp, 4, t->psi_index);
    t->comp->storage_assign(t->comp, 5, t->e_index);
    t->comp->storage_assign(t->comp, 6, t->pos);

    particles_data_destroy(&data);
    return (struct Object*)t;
}