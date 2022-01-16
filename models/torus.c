
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <lib/object.h>
#include "torus.h"
#include <render/program.h>
#include <render/pipeline.h>
#include <render/buffer.h>
#include <lib/linmath.h>
#include <models/triangle.frag.h>
#include <models/torus.vert.h>
#include <models/triangle.frag.spv.h>
#include <models/torus.vert.spv.h>

// x = (R+r cos(psi)) cos phi
// y = (R+r cos(psi)) sin phi
// z = r sin phi
// psi in [0,2pi)
// phi in [-pi,pi)

struct Vertex {
    vec3 col;
    vec3 norm;
    vec3 pos;
};

struct UniformBlock {
    mat4x4 mv;
    mat4x4 mvp;
    mat4x4 norm;
};

struct Torus {
    struct Object base;
    struct UniformBlock uniform;
    struct Pipeline* pl;
    struct BufferManager* b;
    int uniform_buffer_id;
    int model_buffer_id;
    int model; // vao
};

static struct Vertex* init (int* nvertices) {
    float R = 0.5;
    float r = 0.25;
    int i,j,k;
#define N 50
#define M 50
    int n = N;
    int m = M;
    struct Vertex A[N][M];
    struct Vertex* vertices = calloc(n*m*3*2, sizeof(struct Vertex));

    for (i = 0; i < n; i++) {
        float phi = 2.f*M_PI*i/n;
        for (j = 0; j < m; j++) {
            float psi = 2.f*M_PI*j/m-M_PI/2.;

            vec3 v;
            v[0] = (R+r*cosf(psi))*cosf(phi);
            v[1] = (R+r*cosf(psi))*sinf(phi);
            v[2] = r*sinf(psi);

            vec3 v1;
            v1[0] = (R+(r+.1f)*cosf(psi))*cosf(phi);
            v1[1] = (R+(r+.1f)*cosf(psi))*sinf(phi);
            v1[2] = (r+.1f)*sinf(psi);

            vec3_sub(v1, v1, v);
            vec3_scale(v1, v1, 10.f);

            memcpy(A[i][j].pos, v, sizeof(vec3));
            memcpy(A[i][j].norm, v1, sizeof(vec3));
        }
    }

    k = 0;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            /*
            {i,j},{i,j+1}
            {i+1,j}
                    {i,j+1}
            {i+1,j},{i+1,j+1}
            */

            memcpy(&vertices[k++], &A[(i+1)%n][j], sizeof(struct Vertex));
            memcpy(&vertices[k++], &A[i][(j+1)%m], sizeof(struct Vertex));
            memcpy(&vertices[k++], &A[i][j], sizeof(struct Vertex));

            memcpy(&vertices[k++], &A[i][(j+1)%m], sizeof(struct Vertex));
            memcpy(&vertices[k++], &A[(i+1)%n][j], sizeof(struct Vertex));
            memcpy(&vertices[k++], &A[(i+1)%n][(j+1)%m], sizeof(struct Vertex));
        }
    }

    for (i = 0; i < k; i++) {
        vec3 col = {
            rand()/(float)RAND_MAX,
            rand()/(float)RAND_MAX,
            rand()/(float)RAND_MAX
        };
        memcpy(vertices[i].col, col, sizeof(col));
        //vertices[i].col[0] = fmax(0, vertices[i].pos[2]);
    }

    printf("nvertices %d\n", k);
#undef N
#undef M
    *nvertices = k;
    return vertices;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Torus* t = (struct Torus*)obj;
    mat4x4 m, v, p, mv, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_X(m, m, ctx->time);
    mat4x4_rotate_Y(m, m, ctx->time);
    mat4x4_rotate_Z(m, m, ctx->time);

    vec3 eye = {.0f, .0f, 2.f};
    vec3 center = {.0f, .0f, .0f};
    vec3 up = {.0f, 1.f, .0f};
    mat4x4_look_at(v, eye, center, up);

    mat4x4_mul(mv, v, m);

    //mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    //mat4x4_identity(p);
    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 100.f);
    mat4x4_mul(mvp, p, mv);

    mat4x4 norm;
    //mat4x4_invert(norm4, mv);
    //mat4x4_transpose(norm4, norm4);
    //mat3x3 norm;
    //mat3x3_from_mat4x4(norm, norm4);
    //mat3x3_from_mat4x4(norm, mv);
    memcpy(norm, mv, sizeof(norm));
    norm[3][0] = norm[3][1] = norm[3][2] = 0;
    norm[0][3] = norm[1][3] = norm[2][3] = 0;
    norm[3][3] = 1;

    memcpy(t->uniform.mvp, mvp, sizeof(mvp));
    memcpy(t->uniform.mv, mv, sizeof(mv));
    memcpy(t->uniform.norm, norm, sizeof(norm));

    t->pl->start(t->pl);
    t->b->update(t->b, t->uniform_buffer_id, &t->uniform, 0, sizeof(t->uniform));
    t->pl->draw(t->pl, t->model);
}

static void t_free(struct Object* obj) {
    struct Torus* t = (struct Torus*)obj;
    t->pl->free(t->pl);
    t->b->free(t->b);
    free(t);
}

struct Object* CreateTorus(struct Render* r) {
    struct Torus* t = calloc(1, sizeof(struct Torus));
    struct Object base = {
        .draw = t_draw,
        .free = t_free
    };

    struct Vertex* vertices;
    int nvertices;

    struct ShaderCode vertex_shader = {
        .glsl = models_torus_vert,
        .spir_v = models_torus_vert_spv,
        .size = models_torus_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_triangle_frag,
        .spir_v = models_triangle_frag_spv,
        .size = models_triangle_frag_spv_size,
    };

    t->base = base;
    t->b = r->buffer_manager(r);

    vertices = init(&nvertices);

    struct PipelineBuilder* pl = r->pipeline(r);
    t->pl = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->begin_buffer(pl, sizeof(struct Vertex))
        ->buffer_attribute(pl, 3, 3, 4, offsetof(struct Vertex, col))
        ->buffer_attribute(pl, 2, 3, 4, offsetof(struct Vertex, norm))
        ->buffer_attribute(pl, 1, 3, 4, offsetof(struct Vertex, pos))

        ->end_buffer(pl)

        ->enable_depth(pl)

        ->build(pl);

    t->uniform_buffer_id = t->b->create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));
    t->pl->uniform_assign(t->pl, 0, t->uniform_buffer_id);
    t->model_buffer_id = t->b->create(t->b, BUFFER_ARRAY, MEMORY_STATIC, vertices, nvertices*sizeof(struct Vertex));
    t->model = t->pl->buffer_assign(t->pl, 0, t->model_buffer_id);

    //t->model = t->pl->buffer_create(t->pl, BUFFER_ARRAY, MEMORY_STATIC, 0, vertices, nvertices*sizeof(struct Vertex));

    free(vertices);

    return (struct Object*)t;
}
