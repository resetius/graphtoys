
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <lib/object.h>
#include "stl.h"
#include <render/program.h>
#include <render/pipeline.h>
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

struct Stl {
    struct Object base;
    struct UniformBlock uniform;
    struct Pipeline* pl;
};

static struct Vertex* init (int* nvertices) {
    //FILE* f = fopen("skull_art.stl", "rb");
    FILE* f = fopen("tricky_kittens.stl", "rb");
    char header[100];
    int n_triangles;
    int i,j,k;
    struct Vertex* res;
    float mn = 1e10, mx = -1e10;

    fread(header, 1, 80, f);
    header[80] = 0;
    printf("header: '%s'\n", header);
    fread(&n_triangles, 4, 1, f);

    *nvertices = n_triangles*3;
    printf("ntriangles: %d\n", n_triangles);

    res = calloc(*nvertices, sizeof(struct Vertex));

    k = 0;
    for (i = 0; i < n_triangles; i++) {
        float nx, ny, nz;
        int attrs = 0;
        fread(&nx, 4, 1, f);
        fread(&ny, 4, 1, f);
        fread(&nz, 4, 1, f);

        for (j = 0; j < 3; j++) {
            float x, y, z;
            fread(&x, 4, 1, f);
            fread(&y, 4, 1, f);
            fread(&z, 4, 1, f);

            struct Vertex v = {
                {1.0, 1.0, 0.0},
                {nx, ny, nz},
                {x, y, z},
            };
            res[k++] = v;

            mn = fmin(mn, x);
            mn = fmin(mn, y);
            mn = fmin(mn, z);
            mx = fmax(mx, x);
            mx = fmax(mx, y);
            mx = fmax(mx, z);
        }
        fread(&attrs, 2, 1, f); attrs = 0;
        fseek(f, attrs, SEEK_CUR);
    }

    for (i = 0; i < k; i++) {
        vec3_scale(res[i].pos, res[i].pos, 4./(mx-mn));
    }

    fclose(f);
    *nvertices = k;
    return res;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Stl* t = (struct Stl*)obj;
    mat4x4 m, v, p, mv, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_X(m, m, ctx->time);
    mat4x4_rotate_Y(m, m, ctx->time);
    mat4x4_rotate_Z(m, m, ctx->time);

    vec3 eye = {4.0f, 0.f, 0.f};
    vec3 center = {.0f, .0f, .0f};
    vec3 up = {.0f, 1.f, .0f};
    mat4x4_look_at(v, eye, center, up);

    mat4x4_mul(mv, v, m);

    //mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    //mat4x4_identity(p);
    mat4x4_perspective(p, 70./2./M_PI, ctx->ratio, 0.3f, 1000.f);
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

    t->pl->uniform_update(t->pl, 0, &t->uniform, 0, sizeof(t->uniform));
    t->pl->run(t->pl);
}

static void t_free(struct Object* obj) {
    struct Stl* t = (struct Stl*)obj;
    t->pl->free(t->pl);
    free(t);
}

struct Object* CreateStl(struct Render* r) {
    struct Stl* t = calloc(1, sizeof(struct Stl));
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

    vertices = init(&nvertices);

    struct PipelineBuilder* pl = r->pipeline(r);
    t->pl = pl->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->begin_uniform(pl, 0, "MatrixBlock", sizeof(struct UniformBlock))
        ->end_uniform(pl)

        ->begin_buffer(pl, sizeof(struct Vertex))
        ->buffer_data(pl, vertices, nvertices*sizeof(struct Vertex))
        ->buffer_attribute(pl, 3, 3, 4, offsetof(struct Vertex, col))
        ->buffer_attribute(pl, 2, 3, 4, offsetof(struct Vertex, norm))
        ->buffer_attribute(pl, 1, 3, 4, offsetof(struct Vertex, pos))

        ->end_buffer(pl)

        ->enable_depth(pl)

        ->build(pl);


    free(vertices);

    return (struct Object*)t;
}
