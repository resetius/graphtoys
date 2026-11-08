#include <stdlib.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

#include "mandelbulb.h"
#include <models/mandelbulb.vert.h>
#include <models/mandelbulb.frag.h>
#include <models/mandelbulb.vert.spv.h>
#include <models/mandelbulb.frag.spv.h>
#include <lib/object.h>
#include <lib/linmath.h>
#include <lib/config.h>
#include <lib/event.h>

#include <render/pipeline.h>

struct UniformBlock {
    mat4x4 rot;
    mat4x4 inv_rot;
    vec4 T;
    int next;
    float ratio;
    int camera;
    float dist;
};

struct Mandelbulb;
struct KeyConsumer {
    struct EventConsumer cons;
    struct Mandelbulb* t;
};

struct Mandelbulb {
    struct Object base;
    vec3 T;
    struct BufferManager* b;
    int uniform_id;

    int cur_type;
    int n_types;

    // camera mode (key C): the rotation stops, a perspective camera orbits the fractal
    int camera;
    float angle; // auto rotation angle, advances only outside the camera mode
    float last_time;
    float yaw, pitch; // camera orbit around the frozen orientation
    float dist; // camera distance from the center

    struct UniformBlock uniform;
    struct Pipeline* pl;
    struct KeyConsumer cons;

    int model;
};

static const float camera_tan_half_fov = 0.57735f; // tan(60 deg / 2)
static const float camera_orbit_step = 0.02f;
static const float camera_max_dist = 8.0f; // rays march at most 10 units

static void t_left(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->yaw += camera_orbit_step;
    } else {
        t->T[0] += t->T[2]*0.01;
    }
}

static void t_right(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->yaw -= camera_orbit_step;
    } else {
        t->T[0] -= t->T[2]*0.01;
    }
}

static void t_up(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->pitch += camera_orbit_step;
    } else {
        t->T[1] -= t->T[2]*0.01;
    }
}

static void t_down(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->pitch -= camera_orbit_step;
    } else {
        t->T[1] += t->T[2]*0.01;
    }
}

static void t_zoom_in(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->dist /= 1.01;
    } else {
        t->T[2] /= 1.01;
    }
}

static void t_zoom_out(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        t->dist *= 1.01;
        if (t->dist > camera_max_dist) {
            t->dist = camera_max_dist;
        }
    } else {
        t->T[2] *= 1.01;
    }
}

static void key_event(struct EventConsumer* cons, struct InputEvent* ev) {
    struct Mandelbulb* t = ((struct KeyConsumer*)cons)->t;
    if (ev->action == GLFW_PRESS && ev->key == GLFW_KEY_C) {
        t->camera = !t->camera;
    }
}

static void mouse_move_event(struct EventConsumer* cons, struct InputEvent* ev) {
}

static const struct EventConsumerVtbl key_consumer_vtbl = {
    .key_event = key_event,
    .mouse_move_event = mouse_move_event
};

static void t_mode(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    t->cur_type = (t->cur_type + 1) % t->n_types;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    //vec3 T = {0.0f, 0.0f, 2.0f};
    if (!t->camera) {
        t->angle += 0.5*(ctx->time - t->last_time);
    }
    t->last_time = ctx->time;

    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_X(rot, rot, t->angle);
    mat4x4_rotate_Y(rot, rot, t->angle);
    mat4x4_rotate_Z(rot, rot, t->angle);
    if (t->camera) {
        mat4x4_rotate_Y(rot, rot, t->yaw);
        mat4x4_rotate_X(rot, rot, t->pitch);
    }

    //mat3x3 norm;
    //mat3x3_from_mat4x4(norm, rot);

    memcpy(t->uniform.rot, rot, sizeof(rot));
    mat4x4_invert(t->uniform.inv_rot, rot);
    if (t->camera) {
        // T.z is the half-height of the view: at distance 1 from the camera here
        vec4 T = {0.0f, 0.0f, camera_tan_half_fov, 0.0f};
        memcpy(t->uniform.T, T, sizeof(T));
    } else {
        memcpy(t->uniform.T, t->T, sizeof(t->T));
    }
    memcpy(&t->uniform.next, &t->cur_type, 4);
    t->uniform.ratio = ctx->ratio;
    t->uniform.camera = t->camera;
    t->uniform.dist = t->dist;

    pl_start(t->pl);
    buffer_update(t->b, t->uniform_id, &t->uniform, 0, sizeof(t->uniform));
    pl_draw(t->pl, t->model);
}

static void t_free(struct Object* obj) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    pl_free(t->pl);
    buffer_mgr_free(t->b);
    free(t);
}

struct Vertex {
    vec3 pos;
};

static const struct ObjectVtbl object_vtbl = {
    .draw = t_draw,
    .free = t_free,
    .move_left = t_left,
    .move_right = t_right,
    .move_up = t_up,
    .move_down = t_down,
    .zoom_in = t_zoom_in,
    .zoom_out = t_zoom_out,
    .mode = t_mode
};

struct Object* CreateMandelbulb(struct Render* r, struct Config* cfg, struct EventProducer* events) {
    struct Mandelbulb* t = calloc(1, sizeof(struct Mandelbulb));
    struct Vertex vertices[] = {
        {{ 1.0f,-1.0f,0.0f}},
        {{-1.0f,-1.0f,0.0f}},
        {{-1.0f, 1.0f,0.0f}},

        {{ 1.0f, 1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},
        {{-1.0f, 1.0f,0.0f}}
    };
    int n_vertices = 6;

    struct ShaderCode vertex_shader = {
        .glsl =models_mandelbulb_vert,
        .spir_v =models_mandelbulb_vert_spv,
        .size = models_mandelbulb_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = models_mandelbulb_frag,
        .spir_v = models_mandelbulb_frag_spv,
        .size = models_mandelbulb_frag_spv_size,
    };

    t->b = render_buffer_manager(r);
    t->base.vtbl = &object_vtbl;
    t->T[0] = t->T[1] = 0.0; t->T[2] = 2.0;

    struct PipelineBuilder* p = render_pipeline(r);
    t->pl = p
        ->set_bmgr(p, t->b)

        ->begin_program(p)
        ->add_vs(p, vertex_shader)
        ->add_fs(p, fragment_shader)
        ->end_program(p)

        ->uniform_add(p, 0, "MatrixBlock")

        ->begin_buffer(p, sizeof(struct Vertex))
        ->buffer_attribute(p, 1, 3, DATA_FLOAT, offsetof(struct Vertex, pos))
        ->end_buffer(p)

        ->build(p);

    t->uniform_id = buffer_create(t->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, sizeof(struct UniformBlock));
    pl_uniform_assign(t->pl, 0, t->uniform_id);

    int model_buffer_id = buffer_create(t->b, BUFFER_ARRAY, MEMORY_STATIC, vertices, n_vertices*sizeof(struct Vertex));
    t->model = pl_buffer_assign(t->pl, 0, model_buffer_id);

    t->n_types = 4;
    t->cur_type = cfg_geti_def(cfg, "type", 0);
    if (t->cur_type < 0 || t->cur_type >= t->n_types) {
        fprintf(stderr, "mandelbulb: type must be in [0, %d), got %d\n", t->n_types, t->cur_type);
        t->cur_type = 0;
    }

    t->dist = 3.5f; // about the same view as the default orthographic one
    t->cons.t = t;
    t->cons.cons.vtbl = &key_consumer_vtbl;
    ev_subscribe(events, (struct EventConsumer*)&t->cons);
    return (struct Object*)t;
}
