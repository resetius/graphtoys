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

    // camera mode (key C): the rotation stops, a free camera flies around (mouse + WASD);
    // its position and direction are relative to the frozen orientation
    int camera;
    float angle; // auto rotation angle, advances only outside the camera mode
    float last_time;
    float yaw, pitch;
    vec3 pos;
    unsigned char* keys; // pressed keys, indexed by GLFW key codes

    struct UniformBlock uniform;
    struct Pipeline* pl;
    struct KeyConsumer cons;

    int model;
};

static const float camera_tan_half_fov = 0.57735f; // tan(60 deg / 2)
static const float camera_turn_step = 0.02f; // arrows, radians per key press or repeat
static const float camera_mouse_sens = 0.003f; // radians per mouse pixel
static const float camera_max_pitch = 1.55f; // a bit less than 90 deg
static const float camera_speed = 1.0f; // distances to the surface per second
static const float camera_min_de = 1e-3f; // lets the camera move inside the set, where DE is 0
static const float camera_max_dist = 8.0f; // rays march at most 10 units

// CPU copy of DE() from mandelbulb.frag, keep them in sync
static float next_dr(float r, float power, float dr) {
    return powf(r, power-1.0f)*power*dr + 1.0f;
}

static void next_point(vec3 out, const vec3 p, const vec3 p0, float* dr, int type) {
    float x = p[0], y = p[1], z = p[2];
    float r = vec3_len(p);
    if (type == 0) {
        *dr = next_dr(r, 2, *dr);
        out[0] = x*x-y*y-z*z;
        out[1] = 2*x*z;
        out[2] = 2*x*y;
    } else if (type == 1) {
        *dr = next_dr(r, 3, *dr);
        out[0] = x*x*x-3*x*(y*y+z*z);
        out[1] = -y*y*y+3*y*x*x-y*z*z;
        out[2] = z*z*z-3*z*x*x+z*y*y;
    } else if (type == 2) {
        float x2 = x*x, x3 = x*x2, x5 = x3*x2, x7 = x5*x2;
        float y2 = y*y, y3 = y*y2, y5 = y3*y2, y7 = y5*y2;
        float z2 = z*z, z3 = z*z2, z5 = z3*z2, z7 = z5*z2;
        float a = y2+z2, b = z2+x2, c = x2+y2;
        *dr = next_dr(r, 9, *dr);
        out[0] = x7*x2 - 36*x7*a + 126*x5*a*a - 84*x3*a*a*a + 9*x*a*a*a*a;
        out[1] = y7*y2 - 36*y7*b + 126*y5*b*b - 84*y3*b*b*b + 9*y*b*b*b*b;
        out[2] = z7*z2 - 36*z7*c + 126*z5*c*c - 84*z3*c*c*c + 9*z*c*c*c*c;
    } else {
        float C = 2; // A, B and D of the shader are 0
        float x2 = x*x, x3 = x*x2, x4 = x*x3;
        float y2 = y*y, y3 = y*y2, y4 = y*y3;
        float z2 = z*z, z3 = z*z2, z4 = z*z3;
        *dr = next_dr(r, 5, *dr);
        out[0] = x*x4 - 10*x3*(y2 + z2) + 5*x*(y4 + C*y2*z2 + z4);
        out[1] = y*y4 - 10*y3*(z2 + x2) + 5*y*(z4 + C*z2*x2 + x4);
        out[2] = z*z4 - 10*z3*(x2 + y2) + 5*z*(x4 + C*x2*y2 + y4);
    }
    vec3_add(out, out, p0);
}

static float distance_estimate(const vec3 p0, int type) {
    vec3 p = {0, 0, 0};
    float dr = 1.0f;
    for (int i = 1; i < 32; i++) {
        vec3 q;
        next_point(q, p, p0, &dr, type);
        vec3_dup(p, q);
        if (vec3_mul_inner(p, p) > 4) {
            break;
        }
    }
    if (vec3_mul_inner(p, p) <= 4) {
        return 0.0f; // the orbit did not escape: the point is inside the set
    }
    float r = vec3_len(p);
    return 0.5f*logf(r)*r/dr;
}

static void camera_turn(struct Mandelbulb* t, float dyaw, float dpitch) {
    t->yaw += dyaw;
    t->pitch += dpitch;
    if (t->pitch > camera_max_pitch) {
        t->pitch = camera_max_pitch;
    } else if (t->pitch < -camera_max_pitch) {
        t->pitch = -camera_max_pitch;
    }
}

// moves the camera by the held keys; frozen is the stopped auto rotation
static void camera_move(struct Mandelbulb* t, mat4x4 frozen, float dt) {
    if (!t->keys) {
        return;
    }
    vec4 local = {
        t->keys[GLFW_KEY_D] - t->keys[GLFW_KEY_A],
        t->keys[GLFW_KEY_SPACE] - t->keys[GLFW_KEY_LEFT_SHIFT],
        t->keys[GLFW_KEY_S] - t->keys[GLFW_KEY_W],
        0
    };
    if (vec4_len(local) == 0) {
        return;
    }
    vec4_norm(local, local);

    mat4x4 view;
    mat4x4_identity(view);
    mat4x4_rotate_Y(view, view, t->yaw);
    mat4x4_rotate_X(view, view, t->pitch);
    vec4 dir;
    mat4x4_mul_vec4(dir, view, local);

    // the step is a fraction of the distance to the surface: fast far away, slow near it
    vec4 pos = {t->pos[0], t->pos[1], t->pos[2], 1}, world;
    mat4x4_mul_vec4(world, frozen, pos);
    float de = fmaxf(distance_estimate(world, t->cur_type), camera_min_de);
    float step = camera_speed*fminf(dt, 0.1f)*de;
    for (int i = 0; i < 3; i++) {
        t->pos[i] += dir[i]*step;
    }

    float len = vec3_len(t->pos);
    if (len > camera_max_dist) {
        vec3_scale(t->pos, t->pos, camera_max_dist/len);
    }
}

static void t_left(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        camera_turn(t, camera_turn_step, 0);
    } else {
        t->T[0] += t->T[2]*0.01;
    }
}

static void t_right(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        camera_turn(t, -camera_turn_step, 0);
    } else {
        t->T[0] -= t->T[2]*0.01;
    }
}

static void t_up(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        camera_turn(t, 0, camera_turn_step);
    } else {
        t->T[1] -= t->T[2]*0.01;
    }
}

static void t_down(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (t->camera) {
        camera_turn(t, 0, -camera_turn_step);
    } else {
        t->T[1] += t->T[2]*0.01;
    }
}

// A/Z: zoom outside the camera mode; in it A strafes (see camera_move) and Z does nothing
static void t_zoom_in(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (!t->camera) {
        t->T[2] /= 1.01;
    }
}

static void t_zoom_out(struct Object* obj, int mods) {
    struct Mandelbulb* t = (struct Mandelbulb*)obj;
    if (!t->camera) {
        t->T[2] *= 1.01;
    }
}

static void key_event(struct EventConsumer* cons, struct InputEvent* ev) {
    struct Mandelbulb* t = ((struct KeyConsumer*)cons)->t;
    t->keys = ev->mask;
    if (ev->action == GLFW_PRESS && ev->key == GLFW_KEY_C) {
        t->camera = !t->camera;
    }
}

static void mouse_move_event(struct EventConsumer* cons, struct InputEvent* ev) {
    struct Mandelbulb* t = ((struct KeyConsumer*)cons)->t;
    if (t->camera) {
        camera_turn(t, -ev->dx*camera_mouse_sens, -ev->dy*camera_mouse_sens);
    }
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
    float dt = ctx->time - t->last_time;
    if (!t->camera) {
        t->angle += 0.5*dt;
    }
    t->last_time = ctx->time;

    mat4x4 rot;
    mat4x4_identity(rot);
    mat4x4_rotate_X(rot, rot, t->angle);
    mat4x4_rotate_Y(rot, rot, t->angle);
    mat4x4_rotate_Z(rot, rot, t->angle);
    if (t->camera) {
        camera_move(t, rot, dt);
        vec4 pos = {t->pos[0], t->pos[1], t->pos[2], 1}, world;
        mat4x4_mul_vec4(world, rot, pos);
        mat4x4_rotate_Y(rot, rot, t->yaw);
        mat4x4_rotate_X(rot, rot, t->pitch);
        // the shader takes the camera position from the translation of Rot
        rot[3][0] = world[0];
        rot[3][1] = world[1];
        rot[3][2] = world[2];
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

    t->pos[2] = 3.5f; // about the same view as the default orthographic one
    t->cons.t = t;
    t->cons.cons.vtbl = &key_consumer_vtbl;
    ev_subscribe(events, (struct EventConsumer*)&t->cons);
    return (struct Object*)t;
}
