
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <lib/formats/gltf.h>
#include <lib/object.h>
#include <lib/linmath.h>
#include <lib/camera.h>
#include <lib/config.h>
#include <lib/event.h>

#include <render/pipeline.h>

#include <models/base.frag.h>
#include <models/base.vert.h>
#include <models/base.frag.spv.h>
#include <models/base.vert.spv.h>

double glfwGetTime();

struct Vertex {
    vec2 tex;
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

struct Node {
    struct UniformBlock uniform;
    struct Pipeline* pl;

    int n_vertices;
    int index_byte_size;
    int index;

    int model;
    int uniform_id;

    mat4x4 matrix;

    int skip;
    void* tex;
};

struct Model {
    struct Object base;
    struct BufferManager* b;
    struct Camera cam;
    struct CameraEventConsumer cons;

    vec4 light;

    struct Node* nodes;
    int cap_nodes;
    int n_nodes;
};

static void draw_node(struct Model* t, struct Node* n, mat4x4 v, mat4x4 p) {
    mat4x4 m, mv, mvp;
    mat4x4_identity(m);
    memcpy(m, n->matrix, sizeof(n->matrix));

    mat4x4_mul(mv, v, m);

    mat4x4_mul(mvp, p, mv);

    mat4x4 norm;
    memcpy(norm, mv, sizeof(norm));
    norm[3][0] = norm[3][1] = norm[3][2] = 0;
    norm[0][3] = norm[1][3] = norm[2][3] = 0;
    norm[3][3] = 1;

    memcpy(n->uniform.mvp, mvp, sizeof(mvp));
    memcpy(n->uniform.mv, mv, sizeof(mv));
    memcpy(n->uniform.norm, norm, sizeof(norm));

    mat4x4_mul_vec4(n->uniform.light, v, t->light);

    n->pl->start(n->pl);
    if (n->tex) {
        n->pl->use_texture(n->pl, n->tex);
    }
    buffer_update(t->b, n->uniform_id, &n->uniform, 0, sizeof(n->uniform));
    n->pl->draw_indexed(n->pl, n->model, n->index, n->index_byte_size);
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Model* t = (struct Model*)obj;

    mat4x4 v, p;

    t->cam.aspect = ctx->ratio;
    cam_update(v, p, &t->cam);

    for (int i = 0; i < t->n_nodes; i++) {
        if (!t->nodes[i].skip) {
            draw_node(t, &t->nodes[i], v, p);
        }
    }
}

static void t_free(struct Object* obj) {
    struct Model* t = (struct Model*)obj;
    for (int i = 0; i < t->n_nodes; i++) {
        if (!t->nodes[i].skip) {
            t->nodes[i].pl->free(t->nodes[i].pl);
        }
    }
    free(t->nodes);
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
    case GL_UNSIGNED_BYTE: return 1;
    };
    printf("type: %d\n", type);
    verify(type == -1);
    return -1;
}

void load_node(struct Render* r, struct BufferManager* b, struct Node* n, int i, struct Gltf* gltf, void** images, struct ShaderCode* vertex_shader, struct ShaderCode* fragment_shader) {
    // TODO

    int mesh = gltf->nodes[i].mesh;
    memcpy(n->matrix, gltf->nodes[i].matrix, sizeof(n->matrix));

    n->skip = 0;

    int nvertices = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].count;
    int npos = gltf->accessors[gltf->meshes[mesh].primitives[0].position].count;

    if (gltf->meshes[mesh].primitives[0].normal < 0) {
        // TODO: skip
        n->skip = 1;
        return;
    }

    struct Vertex* vertices = malloc(npos*sizeof(struct Vertex));

    //int* indices =
    // TODO: loop on primitives

    int ind_view = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].view;
    int pos_view = gltf->accessors[gltf->meshes[mesh].primitives[0].position].view;
    int norm_view = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].view;
    int material_id = gltf->meshes[mesh].primitives[0].material;
    struct GltfMaterial* material = material_id >= 0
        ? &gltf->materials[material_id]
        : NULL;
    int tex_view = -1;
    int tex_components = -1;
    int tex_type = -1;
    int tex_stride = -1;
    int tex_size = -1;
    if (gltf->meshes[mesh].primitives[0].texcoord[0] >= 0) {
        tex_view = gltf->accessors[gltf->meshes[mesh].primitives[0].texcoord[0]].view;
        tex_components = gltf->accessors[gltf->meshes[mesh].primitives[0].texcoord[0]].components;
        tex_type = gltf->accessors[gltf->meshes[mesh].primitives[0].texcoord[0]].component_type;
        tex_size = tex_components*el_size(tex_type);
        tex_stride = gltf->views[tex_view].stride
            ? gltf->views[tex_view].stride
            : tex_size;
    }

    int pos_components = gltf->accessors[gltf->meshes[mesh].primitives[0].position].components;
    int norm_components = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].components;
    int ind_type = gltf->accessors[gltf->meshes[mesh].primitives[0].indices].component_type;
    int pos_type = gltf->accessors[gltf->meshes[mesh].primitives[0].position].component_type;
    int norm_type = gltf->accessors[gltf->meshes[mesh].primitives[0].normal].component_type;
    char* pos_data = gltf->views[pos_view].data +
        gltf->accessors[gltf->meshes[mesh].primitives[0].position].offset;
    char* norm_data = gltf->views[norm_view].data +
        gltf->accessors[gltf->meshes[mesh].primitives[0].normal].offset;
    char* tex_data = NULL;

    if (tex_view >= 0) {
        tex_data = gltf->views[tex_view].data +
            gltf->accessors[gltf->meshes[mesh].primitives[0].texcoord[0]].offset;
    }

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

    n->index_byte_size = el_size(gltf->accessors[gltf->meshes[mesh].primitives[0].indices].component_type);

    if (material
        && material->has_pbr_metallic_roughness
        && material->pbr_metallic_roughness.has_base_color_texture
        && material->pbr_metallic_roughness.base_color_texture.tex_coord == 0 /*TODO*/)
    {
        int index = material->pbr_metallic_roughness.base_color_texture.index;
        int source = gltf->textures[index].source;
        printf("Use %d '%s' %p\n", source, gltf->images[source].name, gltf->images[source].texture); fflush(stdout);
        n->tex = images[source];
// r->tex_new(r, gltf->images[source].texture, TEX_KTX);
    }

    if (n->index_byte_size == 1) {
        n->index_byte_size = 2;
        unsigned short* new_indices = malloc(ind_size*2);
        for (int i = 0; i < ind_size; i++) {
            new_indices[i] = (unsigned char)indices[i];
        }
        ind_size *= 2;
        n->index = buffer_create(b, BUFFER_INDEX, MEMORY_STATIC, new_indices, ind_size);
        free(new_indices);
    } else {
        n->index = buffer_create(b, BUFFER_INDEX, MEMORY_STATIC, indices, ind_size);
    }

    printf("> nvertices: %d %d %d\n", npos, nvertices, ind_size);
    printf("> pos size/stride: %d %d\n", pos_size, pos_stride);
    printf("> norm size/stride: %d %d\n", norm_size, norm_stride);

    int k = 0;
    for (int i = 0; i < npos; i ++) {
        memcpy(vertices[k].pos, &pos_data[i*pos_stride], pos_size);
        memcpy(vertices[k].norm, &norm_data[i*norm_stride], norm_size);
        if (tex_data) {
            memcpy(vertices[k].tex, &tex_data[i*tex_stride], tex_size);
        }

        if (n->tex) {
            // TODO
            vertices[k].col[0] = 0.0;
            vertices[k].col[1] = 0.0;
            vertices[k].col[2] = 0.0;
        } else if (material && material->has_pbr_metallic_roughness) {
            memcpy(vertices[k].col, material->pbr_metallic_roughness.base_color_factor,
                   3*sizeof(float));
        } else {
            vertices[k].col[0] = 1.0;
            vertices[k].col[1] = 0.0;
            vertices[k].col[2] = 1.0;
        }

        //printf("%f %f %f %f %f %f %d\n",
        //       vertices[k].pos[0], vertices[k].pos[1], vertices[k].pos[2],
        //       vertices[k].norm[0], vertices[k].norm[1], vertices[k].norm[2],
        //       indices[i]
        //    );
        k++;
    }

    // TODO: use indices

    struct PipelineBuilder* pl = r->pipeline(r);
    n->pl = pl
        ->set_bmgr(pl, b)
        ->begin_program(pl)
        ->add_vs(pl, *vertex_shader)
        ->add_fs(pl, *fragment_shader)
        ->end_program(pl)

        ->uniform_add(pl, 0, "MatrixBlock")

        ->begin_buffer(pl, sizeof(struct Vertex))
        ->buffer_attribute(pl, 4, 2, DATA_FLOAT, offsetof(struct Vertex, tex))
        ->buffer_attribute(pl, 3, 3, DATA_FLOAT, offsetof(struct Vertex, col))
        ->buffer_attribute(pl, 2, 3, DATA_FLOAT, offsetof(struct Vertex, norm))
        ->buffer_attribute(pl, 1, 3, DATA_FLOAT, offsetof(struct Vertex, pos))

        ->end_buffer(pl)

        ->begin_sampler(pl, 0)
        // TODO: read sampler properties from Glft
        ->sampler_min_filter(pl, FILTER_LINEAR_MIPMAP_LINEAR)
        ->sampler_wrap_s(pl, WRAP_REPEAT)
        ->sampler_wrap_t(pl, WRAP_REPEAT)
        ->end_sampler(pl)

        ->enable_depth(pl)

        ->build(pl);

    n->uniform_id = buffer_create(b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));
    n->pl->uniform_assign(n->pl, 0, n->uniform_id);

    int model_buffer_id = buffer_create(b, BUFFER_ARRAY, MEMORY_STATIC, vertices, k*sizeof(struct Vertex));
    n->model = pl_buffer_assign(n->pl, 0, model_buffer_id);


    free(vertices);
}

struct Object* CreateGltf(struct Render* r, struct Config* cfg, struct EventProducer* events) {
    struct Model* t = calloc(1, sizeof(struct Model));
    struct Object base = {
        .draw = t_draw,
        .free = t_free,
    };

    struct ShaderCode vertex_shader = {
        .glsl = models_base_vert,
        .spir_v = models_base_vert_spv,
        .size = models_base_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_base_frag,
        .spir_v = models_base_frag_spv,
        .size = models_base_frag_spv_size,
    };

    t->base = base;
    vec4 light = {0, -40, 0, 1};
    vec4_dup(t->light, light);

    const char* fn = cfg_gets_def(cfg, "name", "./assets/khr/scenes/sponza/Sponza01.gltf");

    t->b = r->buffer_manager(r);

    struct Gltf gltf;
    gltf_ctor(&gltf, fn);

    t->cam.fov = 70*M_PI/180;
    t->cam.aspect = 1.77;
    t->cam.znear = 0.3;
    t->cam.zfar = 100000;

    cam_init(&t->cam);

    double t1 = glfwGetTime();

    void** images = malloc(gltf.n_images * sizeof(void*));
    for (int i = 0; i < gltf.n_images; i++) {
        printf("Loading '%s'\n", gltf.images[i].name); fflush(stdout);
        images[i] = r->tex_new(r, gltf.images[i].texture, TEX_KTX);
    }
    // TODO: drop images

    printf("Loaded in %f\n", glfwGetTime() - t1);
    //exit(1);

    for (int i = 0; i < gltf.n_nodes; i++) {

        if (gltf.nodes[i].camera >= 0) {
            // TODO: multiple cameras
            struct GltfCamera* c = &gltf.cameras[gltf.nodes[i].camera];
            if (c->is_perspective) {
                printf("Load camera\n");
                t->cam.fov = c->perspective.yfov;
                t->cam.aspect = c->perspective.aspect;
                t->cam.znear = c->perspective.znear;
                t->cam.zfar = c->perspective.zfar;

                cam_init(&t->cam);
                cam_rotate(&t->cam, gltf.nodes[i].rotation);
                cam_translate(&t->cam, gltf.nodes[i].translation);
            }
        }

        if (! (gltf.nodes[i].mesh >= 0 && gltf.meshes[gltf.nodes[i].mesh].n_primitives > 0) ) {
            continue;
        }

        if (t->n_nodes >= t->cap_nodes) {
            t->cap_nodes = (1+t->cap_nodes)*2;
            t->nodes = realloc(t->nodes, t->cap_nodes*sizeof(struct Node));
        }

        load_node(r, t->b, &t->nodes[t->n_nodes++], i, &gltf, images,
                  &vertex_shader, &fragment_shader);
    }

    gltf_dtor(&gltf);

    cam_event_consumer_init(&t->cons, &t->cam);
    events->subscribe(events, (struct EventConsumer*) &t->cons);

    return (struct Object*)t;
}
