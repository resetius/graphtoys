#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <lib/linmath.h>
#include <font/font.h>

#include <models/particles3.vert.h>
#include <models/particles2.frag.h>
#include <models/particles3_parts.comp.h>
#include <models/particles3_mass.comp.h>
#include <models/particles3_mass_sum.comp.h>
#include <models/particles3_pm.comp.h>
#include <models/particles3_pp.comp.h>
#include <models/particles3_pp_sort.comp.h>
#include <models/particles3.vert.spv.h>
#include <models/particles2.frag.spv.h>
#include <models/particles3_parts.comp.spv.h>
#include <models/particles3_mass.comp.spv.h>
#include <models/particles3_mass_sum.comp.spv.h>
#include <models/particles3_pm.comp.spv.h>
#include <models/particles3_pp.comp.spv.h>
#include <models/particles3_pp_sort.comp.spv.h>

#include <lib/verify.h>
#include <lib/config.h>
#include <lib/tga.h>

#include "particles3.h"
#include "particles_data.h"

struct CompSettings {
    vec4 origin;
    int particles;
    int stage;
    int nn;  // grid nn x nn xnn
    int n;   // log2(n)
    int nlists;
    float h; // l/h
    float l; // length of cube edge
    float rho;
    float rcrit;
};

struct CompPPSettings {
    vec4 origin;
    int particles;
    int cell_size;
    int stage;
    int nn; // chain grid, 32x32x32
    int nlists;
    float h; // l/nn
    float l;
    float rcrit;
};

struct VertBlock {
    mat4x4 mvp;
    vec4 origin;
    vec4 color;
    float h;
    float dt;
    float a;
    float dota;
    int nn;
};

struct Particles {
    struct Object base;
    struct Pipeline* comp_mass;
    struct Pipeline* comp_mass_sum;
    struct Pipeline* comp_parts;
    struct Pipeline* comp;
    struct Pipeline* comp_pp;
    struct Pipeline* comp_pp_sort;
    struct Pipeline* pl;
    struct BufferManager* b;
    struct Render* r;

    int particles; // number of particles
    int single_pass;

    // compute pm
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

    // compute pp
    int pp_enabled;
    struct CompPPSettings comp_pp_set;
    int comp_pp_settings;
    int pp_force;
    int cells;
    int list;

    int counter_density_sort;
    int counter_density;
    int counter_psi;
    int counter_e;
    int counter_pp_sort;
    int counter_pp;
    int counter_pos;
    int counter_frag;
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
    double rho;
    quat q;
    double ax, ay, az;
    double T;
    double expansion;
    int step;

    // labels
    struct Font* font;
    struct Label label;
    float label_color[4];
    int label_enabled;

    // screenshots
    int plot_interval;
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

    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 500000.f);

    //float b,tt,l,r;
    //r = 100000*ctx->ratio; tt = 100000;
    //l = -r; b = -tt;
    //mat4x4_ortho(p, l, r, b, tt, -150000.f, 150000.f);

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

    int groups = t->comp_pp_set.nlists; // 8
    t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
    t->comp_parts->start_compute(t->comp_parts, groups, 1, 1);
    t->r->counter_submit(t->r, t->counter_density_sort);

    t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
    t->comp_mass->start_compute(t->comp_mass, groups, 1, 1);
    t->r->counter_submit(t->r, t->counter_density);

    t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
    t->comp_mass_sum->start_compute(t->comp_mass_sum, t->comp_set.nn/32, t->comp_set.nn/32, 1);
    t->r->counter_submit(t->r, t->counter_density);

    if (t->single_pass) {
        verify(nn == 32);
        int stage = 0;
        t->comp_set.stage = stage;
        t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
        t->comp->start_compute(t->comp, 1, 1, 1);
        t->r->counter_submit(t->r, t->counter_e);
    } else {
        for (int stage = 3; stage <= 10; stage ++) {
            t->comp_set.stage = stage;
            t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);

            int groups = 1;
            if (stage > 2) {
                groups = nn / 32;
            }
            t->comp->start_compute(t->comp, groups, groups, 1);

            if (stage == 2) {
                t->r->counter_submit(t->r, t->counter_density);
            } else if (stage < 10) {
                t->r->counter_submit(t->r, t->counter_psi);
            } else {
                t->r->counter_submit(t->r, t->counter_e);
            }
        }
    }

    if (t->pp_enabled) {
        t->comp_pp_set.stage = 1;
        t->b->update_sync(t->b, t->comp_pp_settings, &t->comp_pp_set, 0, sizeof(t->comp_pp_set), 1);
        t->comp_pp_sort->start_compute(t->comp_pp_sort, 1, 1, 1);
        t->r->counter_submit(t->r, t->counter_pp_sort);

        t->comp_pp_set.stage = 2;
        t->b->update_sync(t->b, t->comp_pp_settings, &t->comp_pp_set, 0, sizeof(t->comp_pp_set), 1);
        //t->comp_pp->start_compute(t->comp_pp, groups, groups, groups);
        //t->comp_pp->start_compute(t->comp_pp, 32, 32, 32);
        t->comp_pp->start_compute(t->comp_pp, 32, 32, 32);
        t->r->counter_submit(t->r, t->counter_pp);
    }

//    int nn = t->comp_set.nn;
//    t->b->read(t->b, t->psi_index, t->psi, 0, nn*nn*nn*sizeof(float));

    t->pl->start(t->pl);
    memcpy(t->vert.mvp, &mvp[0][0], sizeof(mat4x4));
    buffer_update(t->b, t->uniform, &t->vert, 0, sizeof(t->vert));

    t->pl->draw(t->pl, t->indices_vao);
    t->r->counter_submit(t->r, t->counter_pos);
    t->r->counter_submit(t->r, t->counter_frag);

    // dota = -4 pi G / 3 / a / a * rho0
    //double rho0 = 10*t->rho; // TODO
    //double dota = t->vert.dota;
    //double a = t->vert.a;
    //double dt = t->vert.dt;
    //double Lambda = -100;

    //t->vert.a += dt*a*sqrt(8.*M_PI/3.*rho0 - Lambda/a/a);
    //t->vert.dota = a*sqrt(8.*M_PI/3.*rho0 - Lambda/a/a);
    //t->vert.dota += -dt*4*M_PI/3./a/a*rho0;
    //t->vert.a += dt*dota;

    //printf("%e %e %e\n", rho0, t->vert.a, t->vert.dota);

//    t->pl->storage_swap(t->pl, 1, 2);

    if (t->label_enabled) {
        label_set_screen(&t->label, ctx->w, ctx->h);
        label_set_vtext(&t->label, "T: %.2f", t->T);
        label_set_pos(&t->label, 10, ctx->h-50);
        label_render(&t->label);
    }

    if (t->plot_interval >= 0 && t->step%t->plot_interval == 0) {
        char buf[1024];
        void* data;
        int w, h;
        snprintf(buf, sizeof(buf)-1, "particles3.%06d.tga", t->step);
        t->r->screenshot(t->r, &data, &w, &h);
        if (data) {
            tga_write(buf, w, h, data, 4, 4);
            free(data);
        }
    }

    t->T += t->vert.dt;
    t->step += 1;

    if (t->expansion > 0.01) {
        t->vert.a = t->expansion*pow(t->T, 2./3.);
        t->vert.dota = t->expansion*2./3.*pow(t->T, -1./3.);
    }
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->pl->free(t->pl);
    t->comp_parts->free(t->comp_parts);
    t->comp_mass->free(t->comp_mass);
    t->comp_mass_sum->free(t->comp_mass_sum);
    t->comp->free(t->comp);
    t->comp_pp->free(t->comp_pp);
    t->comp_pp_sort->free(t->comp_pp_sort);
    t->b->free(t->b);
    free(t->psi);
    free(t->pos_data);
    free(t->density);
    label_dtor(&t->label);
    font_free(t->font);
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
    struct ShaderCode compute_parts_shader = {
        .glsl = models_particles3_parts_comp,
        .spir_v = models_particles3_parts_comp_spv,
        .size = models_particles3_parts_comp_spv_size,
    };
    struct ShaderCode compute_mass_shader = {
        .glsl = models_particles3_mass_comp,
        .spir_v = models_particles3_mass_comp_spv,
        .size = models_particles3_mass_comp_spv_size,
    };
    struct ShaderCode compute_mass_sum_shader = {
        .glsl = models_particles3_mass_sum_comp,
        .spir_v = models_particles3_mass_sum_comp_spv,
        .size = models_particles3_mass_sum_comp_spv_size,
    };
    struct ShaderCode compute_pp_shader = {
        .glsl = models_particles3_pp_comp,
        .spir_v = models_particles3_pp_comp_spv,
        .size = models_particles3_pp_comp_spv_size,
    };
    struct ShaderCode compute_pp_sort_shader = {
        .glsl = models_particles3_pp_sort_comp,
        .spir_v = models_particles3_pp_sort_comp_spv,
        .size = models_particles3_pp_sort_comp_spv_size,
    };

    t->r = r;
    t->b = r->buffer_manager(r);
    t->comp_parts = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_parts_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")
        ->storage_add(pl, 1, "PosBuffer")
        ->storage_add(pl, 2, "ListBuffer")

        ->build(pl);

    pl = r->pipeline(r);
    t->comp_mass = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_mass_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")
        ->storage_add(pl, 1, "DensityBuffer")
        ->storage_add(pl, 2, "PosBuffer")
        ->storage_add(pl, 3, "ListBuffer")

        ->build(pl);

    pl = r->pipeline(r);
    t->comp_mass_sum = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_mass_sum_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")
        ->storage_add(pl, 1, "DensityBuffer")

        ->build(pl);

    printf("Build comp shader\n");
    pl = r->pipeline(r);
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

        ->build(pl);
    printf("Done\n");

    printf("Build comp pp shader\n");
    pl = r->pipeline(r);

    t->comp_pp = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_pp_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")

        ->storage_add(pl, 1, "CellsBuffer")
        ->storage_add(pl, 2, "PosBuffer")
        ->storage_add(pl, 3, "ForceBuffer")
        ->storage_add(pl, 4, "ListBuffer")

        ->build(pl);

    pl = r->pipeline(r);

    t->comp_pp_sort = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_pp_sort_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")

        ->storage_add(pl, 1, "CellsBuffer")
        ->storage_add(pl, 2, "PosBuffer")
        ->storage_add(pl, 3, "ListBuffer")

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
        ->storage_add(pl, 6, "ForceBuffer")

        ->begin_buffer(pl, 4)
        ->buffer_attribute(pl, 1, 1, DATA_INT, 0)
        ->end_buffer(pl)

        ->geometry(pl, GEOM_POINTS)

        ->build(pl);

    quat_identity(t->q);

    struct ParticlesData data;
    particles_data_init(&data, cfg);

    t->particles = data.n_particles;

    int size = t->particles*4*sizeof(float);

    float x0 = cfg_getf_def(cfg, "x0", -1000);
    float y0 = cfg_getf_def(cfg, "y0", -1000);
    float z0 = cfg_getf_def(cfg, "z0", -1000);
    float l = cfg_getf_def(cfg, "l", 2000);
    t->single_pass = cfg_geti_def(cfg, "single_pass", 0);
    t->pp_enabled = cfg_geti_def(cfg, "pp", 0);
    t->z = z0 + l + cfg_geti_def(cfg, "zoff", 1);
    t->expansion = cfg_getf_def(cfg, "expansion", 0);

    float origin[] = {x0, y0, z0};
    int nn = cfg_geti_def(cfg, "nn", 32);
    float h = l/nn;
    verify (nn % 32 == 0, "nn muste be divided by 32!\n");
    t->comp_set.nn = nn;
    t->comp_set.n = 31-__builtin_clz(nn);
    t->comp_set.h = h;
    t->comp_set.l = l;
    t->comp_set.particles = t->particles;

    memcpy(t->vert.origin, origin, 3*sizeof(float));
    memcpy(t->comp_set.origin, origin, 3*sizeof(float));

    t->vert.h = h;
    t->vert.nn = nn;
    t->vert.dt = cfg_getf_def(cfg, "dt", 0.001);
    t->vert.a = 1;
    t->vert.dota = 0;
    t->T = 1;
    t->vert.color[0] = 1;
    t->vert.color[1] = 0;
    t->vert.color[2] = 1;
    t->vert.color[3] = 1;
    cfg_getv4_def(cfg, t->vert.color, "color", t->vert.color);

    t->density = NULL;
    //t->density = malloc(nn*nn*nn*sizeof(float));
    //distribute(nn, 1, t->density, data.coords, t->particles, h, origin);
    t->density_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL /*t->density*/,
                                    8*nn*nn*nn*sizeof(float));
    t->psi = malloc(nn*nn*nn*sizeof(float));
    t->psi_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL,
                                nn*nn*nn*sizeof(float));

    t->e_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL,
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
    t->comp_pp_settings = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct CompPPSettings));

    t->indices = t->b->create(t->b, BUFFER_ARRAY, MEMORY_STATIC, data.indices, t->particles*sizeof(int));

    t->uniform = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct VertBlock));

    t->pos_data = malloc(size);

    double mass = 0;
    for (int i = 0; i < data.n_particles; i++) {
        mass += data.coords[4*i+3];
    }
    t->rho = mass/(l*l*l);
    t->comp_set.rho = t->rho;
    //printf("%e %e %e\n", t->rho, mass, l*l*l);
    //abort();

    t->pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.coords, size);
    t->new_pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.coords, size);
    t->vel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.vels, size);
    t->accel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, data.accel, size);

    t->comp_pp_set.nn = t->comp_set.nn; // 32;
    memcpy(t->comp_pp_set.origin, t->comp_set.origin, sizeof(t->comp_pp_set.origin));
    t->comp_pp_set.particles = t->particles;
    t->comp_pp_set.cell_size = 512 / (t->comp_pp_set.nn/32);
    t->comp_pp_set.h = l / t->comp_pp_set.nn;
    t->comp_pp_set.l = l;
    t->comp_pp_set.rcrit = t->comp_pp_set.h; // TODO
    t->comp_set.rcrit = t->comp_pp_set.rcrit;
    t->comp_set.nlists = t->comp_pp_set.nlists = 8;

    // TODO: don't allocate it for pp-only
    t->pp_force = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL, size);

    if (t->pp_enabled) {
        int cells_size =
            t->comp_pp_set.nn*t->comp_pp_set.nn*t->comp_pp_set.nn*
            t->comp_pp_set.cell_size*sizeof(int);

        t->cells = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC,NULL,cells_size);
    }

    t->list = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL, (t->particles+64*32*32)*sizeof(int));

    t->indices_vao = t->pl->buffer_assign(t->pl, 0, t->indices);

    t->pl->uniform_assign(t->pl, 0, t->uniform);
    t->pl->storage_assign(t->pl, 1, t->pos);
    t->pl->storage_assign(t->pl, 2, t->new_pos);
    t->pl->storage_assign(t->pl, 3, t->vel);
    t->pl->storage_assign(t->pl, 4, t->accel);
    t->pl->storage_assign(t->pl, 5, t->e_index);
    t->pl->storage_assign(t->pl, 6, t->pp_force);

    t->comp_parts->uniform_assign(t->comp_parts, 0, t->comp_settings);
    t->comp_parts->storage_assign(t->comp_parts, 1, t->pos);
    t->comp_parts->storage_assign(t->comp_parts, 2, t->list);

    t->comp_mass->uniform_assign(t->comp_mass, 0, t->comp_settings);
    t->comp_mass->storage_assign(t->comp_mass, 1, t->density_index);
    t->comp_mass->storage_assign(t->comp_mass, 2, t->pos);
    t->comp_mass->storage_assign(t->comp_mass, 3, t->list);

    t->comp_mass_sum->uniform_assign(t->comp_mass_sum, 0, t->comp_settings);
    t->comp_mass_sum->storage_assign(t->comp_mass_sum, 1, t->density_index);

    t->comp->uniform_assign(t->comp, 0, t->comp_settings);
    t->comp->storage_assign(t->comp, 1, t->fft_table_index);
    t->comp->storage_assign(t->comp, 2, t->work_index);
    t->comp->storage_assign(t->comp, 3, t->density_index);
    t->comp->storage_assign(t->comp, 4, t->psi_index);
    t->comp->storage_assign(t->comp, 5, t->e_index);

    if (t->pp_enabled) {
        t->comp_pp->uniform_assign(t->comp_pp, 0, t->comp_pp_settings);
        t->comp_pp->storage_assign(t->comp_pp, 1, t->cells);
        t->comp_pp->storage_assign(t->comp_pp, 2, t->pos);
        t->comp_pp->storage_assign(t->comp_pp, 3, t->pp_force);
        t->comp_pp->storage_assign(t->comp_pp, 4, t->list);

        t->comp_pp_sort->uniform_assign(t->comp_pp_sort, 0, t->comp_pp_settings);
        t->comp_pp_sort->storage_assign(t->comp_pp_sort, 1, t->cells);
        t->comp_pp_sort->storage_assign(t->comp_pp_sort, 2, t->pos);
        t->comp_pp_sort->storage_assign(t->comp_pp_sort, 3, t->list);
    }
    t->counter_density_sort = r->counter_new(r, "density_sort", COUNTER_COMPUTE);
    t->counter_density = r->counter_new(r, "density", COUNTER_COMPUTE);
    t->counter_psi = r->counter_new(r, "psi", COUNTER_COMPUTE);
    t->counter_e = r->counter_new(r, "e", COUNTER_COMPUTE);
    t->counter_pp_sort = r->counter_new(r, "pp_sort", COUNTER_COMPUTE);
    t->counter_pp = r->counter_new(r, "pp", COUNTER_COMPUTE);
    t->counter_pos = r->counter_new(r, "pos", COUNTER_VERTEX);
    t->counter_frag = r->counter_new(r, "frag", COUNTER_FRAG);

    particles_data_destroy(&data);

    int font_size = cfg_geti_def(cfg, "label_font_size", 150);
    t->font = font_new(r, 0, 16*64, font_size, font_size);
    label_ctor(&t->label, t->font);
    label_set_text(&t->label, "T: ");
    label_set_pos(&t->label, 50, 50);

    t->label_color[0] = 0;
    t->label_color[1] = 1;
    t->label_color[2] = 1;
    t->label_color[3] = 1;
    cfg_getv4_def(cfg, t->label_color, "label_color", t->label_color);
    label_set_color(&t->label, t->label_color);
    t->label_enabled = cfg_geti_def(cfg, "label_show", 0);

    t->plot_interval = cfg_geti_def(cfg, "plot_interval", -1);

    return (struct Object*)t;
}
