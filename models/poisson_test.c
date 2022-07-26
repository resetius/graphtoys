#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <models/particles3_poisson.comp.spv.h>

#include <lib/verify.h>
#include <lib/config.h>
#include <lib/linmath.h>
#include <lib/object.h>

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

struct Particles {
    struct Object base;
    struct Pipeline* comp_poisson;
    struct BufferManager* b;
    struct Render* r;

    int single_pass;

    // compute pm
    struct CompSettings comp_set;

    int density_index;
    float* density;
    int psi_index;
    float* psi;
    int fft_table_index;
    int work_index;
    int comp_settings;
    //

    int model;
    float z;
    double rho;
    quat q;
    double ax, ay, az;
    double T;
};

static void draw_(struct Object* obj, struct DrawContext* ctx) {
    struct Particles* t = (struct Particles*)obj;

    if (t->single_pass) {
        int stage = 0;
        t->comp_set.stage = stage;
        t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
        t->comp_poisson->start_compute(t->comp_poisson, 1, 1, 1);
    } else {
        for (int stage = 1; stage <= 7; stage ++) {
            t->comp_set.stage = stage;
            t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);

            int groups = t->comp_set.nn / 32;
            t->comp_poisson->start_compute(t->comp_poisson, groups, groups, 1);
        }
    }
}

static void free_(struct Object* obj) {
    struct Particles* t = (struct Particles*)obj;
    t->comp_poisson->free(t->comp_poisson);
    t->b->free(t->b);
    free(t->psi);
    free(t->density);
    free(obj);
}

struct Object* CreatePoissonTest(struct Render* r, struct Config* cfg) {
    struct Particles* t = calloc(1, sizeof(struct Particles));
    struct Object base = {
        .draw = draw_,
        .free = free_
    };
    t->base = base;

    struct PipelineBuilder* pl = r->pipeline(r);
    struct ShaderCode compute_shader = {
        .spir_v = models_particles3_poisson_comp_spv,
        .size = models_particles3_poisson_comp_spv_size,
    };

    t->r = r;
    t->b = r->buffer_manager(r);

    printf("Build comp shader\n");
    t->comp_poisson = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_cs(pl, compute_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "Settings")

        ->storage_add(pl, 1, "FFTBuffer")
        ->storage_add(pl, 2, "WorkBuffer")
        ->storage_add(pl, 3, "DensityBuffer")
        ->storage_add(pl, 4, "PotentialBuffer")

        ->build(pl);
    printf("Done\n");

    float origin[4] = {0,0,0,0};
    float l = 2*M_PI;
    t->single_pass = cfg_geti_def(cfg, "single_pass", 0);

    int nn = cfg_geti_def(cfg, "nn", 32);
    verify(!t->single_pass || nn == 32);
    float h = l/nn;
    verify (nn % 32 == 0, "nn muste be divided by 32!\n");
    t->comp_set.nn = nn;
    t->comp_set.n = 31-__builtin_clz(nn);
    t->comp_set.h = h;
    t->comp_set.l = l;

    t->density = NULL;
    //t->density = malloc(nn*nn*nn*sizeof(float));
    //distribute(nn, 1, t->density, data.coords, t->particles, h, origin);
    t->density_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL /*t->density*/,
                                    8*nn*nn*nn*sizeof(float));
    t->psi = malloc(nn*nn*nn*sizeof(float));
    t->psi_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL,
                                nn*nn*nn*sizeof(float));

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

    t->comp_poisson->uniform_assign(t->comp_poisson, 0, t->comp_settings);
    t->comp_poisson->storage_assign(t->comp_poisson, 1, t->fft_table_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 2, t->work_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 3, t->density_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 4, t->psi_index);

    return (struct Object*)t;
}
