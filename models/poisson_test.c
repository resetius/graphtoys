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

static double ans1(int i, int k, int j, double dz, double dy, double dx, double x1, double y1, double z1, double x2, double y2, double z2) {
    double x = x1+dx*j-dx/2;
    double y = y1+dy*k-dy/2;
    double z = z1+dz*i-dz/2;

    double sx = sin(x);
    double cy = cos(y);
    double sz = sin(z);
    return sx*sx*sx+cy*cy*cy*cy*cy-sz;
}

static double rp1(int i, int k, int j, double dz, double dy, double dx, double x1, double y1, double z1, double x2, double y2, double z2) {
    double x = x1+dx*j-dx/2;
    double y = y1+dy*k-dy/2;
    double z = z1+dz*i-dz/2;

    return 1./16. *
        (-12.* sin(x) + 36.* sin(3* x)
         - 10.* cos(y) - 45.* cos(3* y)
         - 25.* cos(5* y) + 16.* sin(z));
}

static void draw_(struct Object* obj, struct DrawContext* ctx) {
    struct Particles* t = (struct Particles*)obj;
    int nn = t->comp_set.nn;
    float h = t->comp_set.h;
    float l = t->comp_set.l;

    if (t->single_pass) {
        int stage = 0;
        t->comp_set.stage = stage;
        t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);
        t->comp_poisson->start_compute(t->comp_poisson, 1, 1, 1);
    } else {
        for (int stage = 1; stage <= 7; stage ++) {
            t->comp_set.stage = stage;
            t->b->update_sync(t->b, t->comp_settings, &t->comp_set, 0, sizeof(t->comp_set), 1);

            int groups = nn / 32;
            t->comp_poisson->start_compute(t->comp_poisson, groups, groups, 1);
        }
    }

    // OpenGL only yet!
    // check accuracy and exit
    t->b->read(t->b, t->psi_index, t->psi, 0, nn*nn*nn*sizeof(float));
#define off(i,k,j) ((i)*nn*nn+(k)*nn+(j))
#define poff(i,k,j) (((i+nn)%nn)*nn*nn+((k+nn)%nn)*nn+((j+nn)%nn))
    double nrm = 0;
    double nrm1= 0;
    double avg = 0.0;
    for (int i = 0; i < nn; i++) {
        for (int k = 0; k < nn; k++) {
            for (int j = 0; j < nn; j++) {
                avg += t->psi[off(i,k,j)];
            }
        }
    }
    avg /= nn*nn*nn;
    for (int i = 0; i < nn; i++) {
        for (int k = 0; k < nn; k++) {
            for (int j = 0; j < nn; j++) {
                t->psi[off(i,k,j)] -= avg;
            }
        }
    }

    for (int i = 0; i < nn; i++) {
        for (int k = 0; k < nn; k++) {
            for (int j = 0; j < nn; j++) {
                double f = ans1(i, k, j, h, h, h, 0, 0, 0, l, l, l);
                nrm = fmax(nrm, fabs(t->psi[off(i,k,j)]-f));
                nrm1 = fmax(nrm1, fabs(f));
            }
        }
    }
    nrm /= nrm1;
    fprintf(stderr, "err = '%e'\n", nrm);
#undef off
#undef poff
    exit(0);
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
    t->density = malloc(nn*nn*nn*sizeof(float));
#define off(i,k,j) ((i)*nn*nn+(k)*nn+(j))
    for (int i = 0; i < nn; i++) {
        for (int k = 0; k < nn; k++) {
            for (int j = 0; j < nn; j++) {
                t->density[off(i,k,j)] = rp1(i,k,j,h,h,h,0,0,0,l,l,l);
            }
        }
    }
#undef off
    t->density_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC,
                                    t->density,
                                    nn*nn*nn*sizeof(float));
    t->psi = malloc(nn*nn*nn*sizeof(float));
    t->psi_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_READ, NULL,
                                nn*nn*nn*sizeof(float));
    int n = t->comp_set.n;
    int fft_table_size = (2*nn+2*nn+nn+(n+1)*nn)*sizeof(float);
    float* fft_table = malloc(fft_table_size);
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
#define off(k,l) ((l+1)*nn+(k-1))
    for (int l = -1; l <= n-1; l++) {
        for (int k = 1; k <= (1<<(l+1)); k++) {
            double x = 2.*cos(M_PI*(2*k-1)/(1<<(l+2)));
            x = 1./x;
            fft_table[m+off(k,l)] = x;
        }
    }
#undef off

    t->fft_table_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, fft_table, fft_table_size);
    free(fft_table);
    // TODO: improve memory usage
    // 1 -- input
    // 2 -- output
    // 3 -- temporary buffer 1
    // 4 -- temporary buffer 2
    t->work_index = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_STATIC, NULL, 4*nn*nn*nn*sizeof(float));
    t->comp_settings = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct CompSettings));

    t->comp_poisson->uniform_assign(t->comp_poisson, 0, t->comp_settings);
    t->comp_poisson->storage_assign(t->comp_poisson, 1, t->fft_table_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 2, t->work_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 3, t->density_index);
    t->comp_poisson->storage_assign(t->comp_poisson, 4, t->psi_index);

    return (struct Object*)t;
}
