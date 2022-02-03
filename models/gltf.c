
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <lib/formats/gltf.h>
#include <lib/object.h>
#include <lib/linmath.h>
#include <lib/config.h>

#include <render/pipeline.h>

#include <models/stl.frag.h>
#include <models/stl.vert.h>
#include <models/stl.frag.spv.h>
#include <models/stl.vert.spv.h>

#include <models/dot.frag.h>
#include <models/dot.vert.h>
#include <models/dot.frag.spv.h>
#include <models/dot.vert.spv.h>

// x = (R+r cos(psi)) cos phi
// y = (R+r cos(psi)) sin phi
// z = r sin phi
// psi in [0,2pi)
// phi in [-pi,pi)

struct Vertex {
    vec3 col;
    vec3 pos;
    vec3 norm;
};

struct UniformBlock {
    mat4x4 mv;
    mat4x4 mvp;
    mat4x4 norm;
    vec4 light;
};

struct Model {
    struct Object base;
    struct UniformBlock uniform;
    struct Pipeline* pl;
    struct Pipeline* lamp;
    struct BufferManager* b;
    //mat4x4 m;
    float ax, ay, az;
    vec4 light;
    quat q;

    int n_vertices;
    int index_byte_size;
    int index;

    int model;
    int uniform_id;
    int dot;
    int dot_uniform_id;
};

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Model* t = (struct Model*)obj;
    mat4x4 m, v, p, mv, mvp;

    mat4x4_identity(m);
    //mat4x4_rotate_X(m, m, ctx->time);
    //mat4x4_rotate_Y(m, m, ctx->time);
    //mat4x4_rotate_Z(m, m, ctx->time);
    //memcpy(m, t->m, sizeof(m));
    mat4x4_from_quat(m, t->q);

    vec3 eye = {0.0f, -50.0f, 0.f};
    vec3 center = {.0f, .0f, .0f};
    vec3 up = {.0f, .0f, -.1f};

    //vec3 eye = {0.0f, -1.0f, 0.f};
    //vec3 center = {.0f, .0f, .0f};
    //vec3 up = {.0f, .0f, -.1f};

    mat4x4_look_at(v, eye, center, up);

    //mat4x4_rotate_X(v, v, ctx->time);
    //mat4x4_rotate_Y(v, v, ctx->time);
    //mat4x4_rotate_Z(v, v, ctx->time);

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

    //mat4x4 lrot;
    //mat4x4_identity(lrot);
    //mat4x4_rotate_X(lrot, lrot, ctx->time);
    //vec4 light = {10, -10, 20, 0};
    //mat4x4_mul_vec4(t->uniform.light, lrot, light);
    //printf("%f %f %f %f\n, ", t->uniform.light[0],t->uniform.light[1],t->uniform.light[2],t->uniform.light[3]);
    //memcpy(t->uniform.light, light, sizeof(light));

    // light position in view coordinate
    mat4x4_mul_vec4(t->uniform.light, v, t->light);

    t->pl->start(t->pl);
    buffer_update(t->b, t->uniform_id, &t->uniform, 0, sizeof(t->uniform));
    //t->pl->draw(t->pl, t->model);
    t->pl->draw_indexed(t->pl, t->model, t->index, t->index_byte_size);

    mat4x4 m1;
    mat4x4_identity(m1);
    mat4x4_translate_in_place(m1, t->light[0], t->light[1], t->light[2]);
    mat4x4_rotate_X(m1, m1, M_PI/2);

    mat4x4_mul(mv, v, m1);
    mat4x4_mul(mvp, p, mv);

    t->lamp->start(t->lamp);
    buffer_update(t->b, t->dot_uniform_id, mvp, 0, sizeof(mvp));
    t->lamp->draw(t->lamp, t->dot);
}

static void transform(struct Model* t) {
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
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[0] -= 0.1;
    } else {
        t->az += -5*M_PI/360;
    }
    transform(t);
}

static void move_right(struct Object* obj, int mods) {
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[0] += 0.1;
    } else {
        t->az += 5*M_PI/360;
    }
    transform(t);
}

static void move_up(struct Object* obj, int mods) {
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[2] += 0.1;
    } else {
        t->ax += -5*M_PI/360;
    }
    transform(t);
}

static void move_down(struct Object* obj, int mods) {
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[2] -= 0.1;
    } else {
        t->ax += 5*M_PI/360;
    }
    transform(t);
}

static void zoom_in(struct Object* obj, int mods) {
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[1] += 0.1;
    } else {
        t->ay += 5*M_PI/360;;
    }
    transform(t);
}

static void zoom_out(struct Object* obj, int mods) {
    struct Model* t = (struct Model*)obj;

    if (mods & 1) {
        t->light[1] -= 0.1;
    } else {
        t->ay += -5*M_PI/360;
    }
    transform(t);
}

static void t_free(struct Object* obj) {
    struct Model* t = (struct Model*)obj;
    t->pl->free(t->pl);
    t->lamp->free(t->lamp);
    t->b->free(t->b);
    free(t);
}

#include <glad/gl.h>
#include <lib/verify.h>

static int el_size(int type) {
    switch (type) {
    case GL_FLOAT: return 4;
    case GL_UNSIGNED_SHORT: return 2;
    case GL_UNSIGNED_INT: return 4;
    };
    verify(type == -1);
    return -1;
}

struct Object* CreateGltf(struct Render* r, struct Config* cfg) {
    struct Model* t = calloc(1, sizeof(struct Model));
    struct Object base = {
        .draw = t_draw,
        .free = t_free,
        .move_left = move_left,
        .move_right = move_right,
        .move_up = move_up,
        .move_down = move_down,
        .zoom_in = zoom_in,
        .zoom_out = zoom_out,
    };

    struct Vertex* vertices;
    int nvertices;

    struct ShaderCode vertex_shader = {
        .glsl = models_stl_vert,
        .spir_v = models_stl_vert_spv,
        .size = models_stl_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_stl_frag,
        .spir_v = models_stl_frag_spv,
        .size = models_stl_frag_spv_size,
    };

    struct ShaderCode vertex_shadert = {
        .glsl = models_dot_vert,
        .spir_v = models_dot_vert_spv,
        .size = models_dot_vert_spv_size,
    };
    struct ShaderCode fragment_shadert = {
        .glsl = models_dot_frag,
        .spir_v = models_dot_frag_spv,
        .size = models_dot_frag_spv_size,
    };

    //mat4x4_identity(t->m);
    quat_identity(t->q);

    t->base = base;
    vec4 light = {0, -40, 0, 1};
    vec4_dup(t->light, light);

    const char* fn = cfg_gets_def(cfg, "name", "skull_art.stl");
    //FILE* f = fopen("tricky_kittens.stl", "rb");
    //FILE* f = fopen("rocklobster_solid.stl", "rb");
    //FILE* f = fopen("Doraemon_Lucky_Cat.stl", "rb");


    t->b = r->buffer_manager(r);

    struct Gltf* gltf = gltf_load(fn);

    // TODO
    int mesh = 0;
    nvertices = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].count;
    int npos = gltf->accessors[gltf->meshes[mesh].primitives[0].position].count;

    vertices = malloc(npos*sizeof(struct Vertex));

    //int* indices =

    int ind_view = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].view;
    int pos_view = gltf->accessors[gltf->meshes[mesh].primitives[0].position].view;
    int norm_view = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].view;

    int pos_components = gltf->accessors[gltf->meshes[mesh].primitives[0].position].components;
    int norm_components = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].components;
    int ind_type = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].component_type;
    int pos_type = gltf->accessors[gltf->meshes[mesh].primitives[0].position].component_type;
    int norm_type = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].component_type;
    char* pos_data = gltf->views[pos_view].data +
        gltf->accessors[gltf->meshes[mesh].primitives[0].position].offset;
    char* norm_data = gltf->views[norm_view].data +
        gltf->accessors[gltf->meshes[mesh].primitives[0].normal].offset;

    int pos_size = pos_components*el_size(pos_type);
    int norm_size = norm_components*el_size(norm_type);
    int ind_size = nvertices*el_size(ind_type);
    int pos_stride = gltf->views[pos_view].stride
            ? gltf->views[pos_view].stride
            : pos_size;
    int norm_stride =
        gltf->views[norm_view].stride
            ? gltf->views[norm_view].stride
            : norm_size;

    printf("indview: %d\n", ind_view);
    char* indices = gltf->views[ind_view].data+gltf->accessors[gltf->meshes[mesh].primitives[0].indices].offset;

    t->index = buffer_create(t->b, BUFFER_INDEX, MEMORY_STATIC, indices, ind_size);
    t->index_byte_size = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].component_type == 5123
        ? 2: 4;

    printf("> nvertices: %d %d %d\n", npos, nvertices, ind_size);
    printf("> pos size/stride: %d %d\n", pos_size, pos_stride);
    printf("> norm size/stride: %d %d\n", norm_size, norm_stride);

    int k = 0;
    for (int i = 0; i < npos; i ++) {
        memcpy(vertices[k].pos, &pos_data[i*pos_stride], pos_size);
        memcpy(vertices[k].norm, &norm_data[i*norm_stride], norm_size);
        vertices[k].col[0] = 1.0;
        vertices[k].col[1] = 0.0;
        vertices[k].col[2] = 1.0;

        //printf("%f %f %f %f %f %f %d\n",
        //       vertices[k].pos[0], vertices[k].pos[1], vertices[k].pos[2],
        //       vertices[k].norm[0], vertices[k].norm[1], vertices[k].norm[2],
        //       indices[i]
        //    );
        k++;
    }

    // TODO: use indices


    gltf_destroy(gltf);

    struct PipelineBuilder* pl = r->pipeline(r);
    t->pl = pl
        ->set_bmgr(pl, t->b)
        ->begin_program(pl)
        ->add_vs(pl, vertex_shader)
        ->add_fs(pl, fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->begin_buffer(pl, sizeof(struct Vertex))
        ->buffer_attribute(pl, 3, 3, DATA_FLOAT, offsetof(struct Vertex, col))
        ->buffer_attribute(pl, 2, 3, DATA_FLOAT, offsetof(struct Vertex, norm))
        ->buffer_attribute(pl, 1, 3, DATA_FLOAT, offsetof(struct Vertex, pos))

        ->end_buffer(pl)

        ->enable_depth(pl)

        ->build(pl);

    t->uniform_id = buffer_create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));
    t->pl->uniform_assign(t->pl, 0, t->uniform_id);

    int model_buffer_id = buffer_create(t->b, BUFFER_ARRAY, MEMORY_STATIC, vertices, k*sizeof(struct Vertex));
    t->model = pl_buffer_assign(t->pl, 0, model_buffer_id);

    struct Vertex pp[] = {
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, { -1,  1, 0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, { -1, -1, 0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {  1, -1, 0}},

        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, { -1,  1, 0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {  1, -1, 0}},
        {{1.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {  1,  1, 0}}
    };

    struct PipelineBuilder* lamp = r->pipeline(r);
    t->lamp = lamp
        ->set_bmgr(lamp, t->b)
        ->begin_program(lamp)
        ->add_vs(lamp, vertex_shadert)
        ->add_fs(lamp, fragment_shadert)
        ->end_program(lamp)

        ->uniform_add(lamp, 0, "MatrixBlock")

        ->begin_buffer(lamp, sizeof(struct Vertex))
        ->buffer_attribute(lamp, 2, 3, DATA_FLOAT, offsetof(struct Vertex, col))
        ->buffer_attribute(lamp, 1, 3, DATA_FLOAT, offsetof(struct Vertex, pos))

        ->end_buffer(lamp)

        ->enable_blend(lamp)
        ->enable_depth(lamp)

        ->build(lamp);

    t->dot_uniform_id = buffer_create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(mat4x4));
    t->lamp->uniform_assign(t->lamp, 0, t->dot_uniform_id);

    int dot_buffer_id = buffer_create(t->b, BUFFER_ARRAY, MEMORY_STATIC, pp, sizeof(pp));
    t->dot = pl_buffer_assign(t->lamp, 0, dot_buffer_id);

    free(vertices);

    return (struct Object*)t;
}
