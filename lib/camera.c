#include <stdio.h>

#include "camera.h"

void cam_init(struct Camera* cam) {
    vec3 eye = {0.0f, 0.0f, 0.f};
    vec3 center = {.0f, .0f, -1.0f};
    vec3 up = {.0f, 1.f, .0f};

    memcpy(cam->eye, eye, sizeof(eye));
    memcpy(cam->center, center, sizeof(center));
    memcpy(cam->up, up, sizeof(up));

    cam->fov = 70*M_PI/180.;
    cam->aspect = 1.77;
    cam->znear = 0.3;
    cam->zfar = 100000;
}

void cam_update(mat4x4 v, mat4x4 p, struct Camera* cam) {
    mat4x4_look_at(v, cam->eye, cam->center, cam->up);
    mat4x4_perspective(p, cam->fov, cam->aspect, cam->znear, cam->zfar);
}

void cam_rotate(struct Camera* cam, quat q) {
    vec3 v = {0.0, 0.0, 0.0};
    float len;
    vec3_sub(v, cam->center, cam->eye);
    len = vec3_len(v);

    quat_mul_vec3(v, q, v);

    if (fabs(vec3_mul_inner(v, cam->up) / len / vec3_len(cam->up)) > 0.95) {
        return;
    }

    vec3_add(cam->center, cam->center, v);

    // preserve eye->center len
    // v = center-eye, center = eye+v
    vec3_sub(v, cam->center, cam->eye);
    vec3_norm(v, v);
    vec3_scale(v, v, len);
    vec3_add(cam->center, cam->eye, v);
}

void cam_translate(struct Camera* cam, vec3 t) {
    memcpy(cam->eye, t, sizeof(cam->eye));
}

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

static void process_key(struct Camera* cam, int key)
{
    vec3 dir;
    vec3 dir1;
    vec3_sub(dir, cam->center, cam->eye);
    float len = vec3_len(dir);
    vec3_scale(dir1, dir, 1.0/len);
    float step = 10000/len;

    vec3 dir2;
    vec3_mul_cross(dir2, dir, cam->up);
    vec3_norm(dir2, dir2);

    vec3 dir3;
    vec3_norm(dir3, cam->up);

    float angle = -10*M_PI/180;
    quat rot1;
    quat_identity(rot1);

    switch (key) {
    case GLFW_KEY_S:
        step = -step;
    case GLFW_KEY_W:
        vec3_scale(dir1, dir1, step);
        vec3_add(cam->center, cam->center, dir1);
        vec3_add(cam->eye, cam->eye, dir1);
        break;
    case GLFW_KEY_A:
        step = -step;
    case GLFW_KEY_D:
        vec3_scale(dir2, dir2, step);
        vec3_add(cam->center, cam->center, dir2);
        vec3_add(cam->eye, cam->eye, dir2);
        break;

    case GLFW_KEY_C:
        step = -step;
    case GLFW_KEY_SPACE:
        vec3_scale(dir3, dir3, step);
        vec3_add(cam->center, cam->center, dir3);
        vec3_add(cam->eye, cam->eye, dir3);
        break;

    case GLFW_KEY_LEFT:
        angle = -angle;
    case GLFW_KEY_RIGHT:
        quat_rotate(rot1, angle, cam->up);
        cam_rotate(cam, rot1);
        break;
    case GLFW_KEY_UP:
        angle = -angle;
    case GLFW_KEY_DOWN:
        quat_rotate(rot1, angle, dir2);
        cam_rotate(cam, rot1);
    default:
        break;
    }
}

static void cam_key_event(struct EventConsumer* cons, int key, int scancode, int action, int mods, unsigned char* mask)
{
    struct Camera* cam = ((struct CameraEventConsumer*)cons)->cam;
    // TODO: keys remapping
    //printf("%f %f %f %f %f %f %f %f %f\n",
    //       cam->center[0], cam->center[1], cam->center[2],
    //       cam->eye[0], cam->eye[1], cam->eye[2],
    //       cam->up[0], cam->up[1], cam->up[2]);
    if (action == GLFW_RELEASE) {
        return;
    }

    int check[] = {
        GLFW_KEY_S, GLFW_KEY_W, GLFW_KEY_D, GLFW_KEY_A,
        GLFW_KEY_SPACE, GLFW_KEY_LEFT, GLFW_KEY_RIGHT,
        GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_C
    };

    for (int i = 0; i < sizeof(check)/sizeof(check[0]); i++) {
        if (mask[check[i]]) {
            process_key(cam, check[i]);
        }
    }
}

void cam_event_consumer_init(struct CameraEventConsumer* cons, struct Camera* cam) {
    struct EventConsumer base = {
        .key_event = cam_key_event
    };
    cons->base = base;
    cons->cam = cam;
}
