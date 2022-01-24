#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <render/render.h>
#include <render/pipeline.h>

#include <lib/linmath.h>
#include <lib/config.h>

#include <models/particles2.vert.h>
#include <models/particles2.frag.h>
#include <models/particles2.vert.spv.h>
#include <models/particles2.frag.spv.h>

#include "particles.h"

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
    t->dirty = 4;
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
    t->dirty = 4;
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
    t->dirty = 4;
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

struct body
{
    double x[3];
    double v[3];
    double mass;
    int fixed;
    char name[100];
};

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

    //int n_x = 16, n_y = 16, n_z = 16;
    int n_x = cfg_geti_def(cfg, "nx", 23);
    int n_y = cfg_geti_def(cfg, "ny", 23);
    int n_z = cfg_geti_def(cfg, "nz", 23);
    int n_particles = n_x*n_y*n_z;
    int size = n_particles*4*sizeof(float);
    float* coords = malloc(size);
    int* indices = malloc(n_particles*sizeof(int));
    float* vels = calloc(1, size);
    float* accel = calloc(1, size);
    int i,j,k,n=0;
    //float side = 4.0f;
    float side = 7.5f;
    float dx = side/(n_x-1);
    float dy = side/(n_y-1);
    float dz = side/(n_z-1);

    quat_identity(t->q);

    srand(time(NULL));

    const double G = 2.92e-6;
    const char* name = cfg_gets_def(cfg, "name", "solar");

    if (!strcmp(name, "sunearth")) {

        struct body body[] = {
            {{0, 0, 0}, {0, 0, 0}, 10000, 1, "Sun"},
            {{0, 1, 0}, {-1, 0, 0}, 1, 0, "Earth"},
        };
        const int nbodies = 2;

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
    } else if (!strcmp(name, "solar")) {

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
    } else if (!strcmp(name, "cube")) {

        for (i = 0; i < n_x; i++) {
            for (j = 0; j < n_y; j++) {
                for (k = 0; k < n_z; k++) {
                    coords[n] = dx * i - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
                    coords[n+1] = dy * j - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
                    coords[n+2] = dz * k - side/2 + (0.5 * (double)rand() / (double)RAND_MAX - 0.25);
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
    } else { // sphere

        while (n < 4*n_x*n_y*n_z) {
            double x = 2*side*(double)rand() / RAND_MAX - side;
            double y = 2*side*(double)rand() / RAND_MAX - side;
            double z = 2*side*(double)rand() / RAND_MAX - side;

            if (x*x+y*y+z*z<side*side) {
                coords[n] = x;
                coords[n+1] = y;
                coords[n+2] = z;
                coords[n+3] = 0.2 + 1.5*(double)rand() / (double)RAND_MAX;

                double R =
                    coords[n]*coords[n]+
                    coords[n+1]*coords[n+1]+
                    coords[n+2]*coords[n+2];
                R = sqrt(R);
                double V = sqrt(200)/sqrt(R); // sqrt(100/R);

                vels[n] = V*coords[n+1];
                vels[n+1] = -V*coords[n];
                vels[n+2] = 0;

                vels[n+3] = 0;
                n += 4;
            }
        }

    }

    t->z = 1.0f;
    t->particles = n/4;

    for (i = 0; i < t->particles; i++) {
        indices[i] = i;
    }

/*
    for (i = 0; i < t->particles; i++) {
        printf("%f %f %f %f, %f %f %f %f\n",
               coords[i*4+0],coords[i*4+1],coords[i*4+2],coords[i*4+3],
               vels[i*4+0],vels[i*4+1],vels[i*4+2],vels[i*4+3]
            );
    }
*/
    size = t->particles*4*sizeof(float);

    t->dirty = 4;
    t->indices = t->b->create(t->b, BUFFER_ARRAY, MEMORY_STATIC, indices, t->particles*sizeof(int));

    t->uniform = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));

    t->pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, coords, size);
    t->vel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, vels, size);
    t->accel = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC_COPY, accel, size);
    t->new_pos = t->b->create(t->b, BUFFER_SHADER_STORAGE, MEMORY_DYNAMIC, coords, size);

    t->indices_vao = t->pl->buffer_assign(t->pl, 0, t->indices);

    t->pl->uniform_assign(t->pl, 0, t->uniform);

    t->pl->storage_assign(t->pl, 1, t->pos);
    t->pl->storage_assign(t->pl, 2, t->vel);
    t->pl->storage_assign(t->pl, 3, t->accel);
    t->pl->storage_assign(t->pl, 4, t->new_pos);


    free(coords);
    free(vels);
    free(accel);
    return (struct Object*)t;
}
