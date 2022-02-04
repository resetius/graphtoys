#include "camera.h"

void cam_init(struct Camera* cam) {
    vec3 eye = {0.0f, 0.0f, 0.f};
    vec3 center = {.0f, .0f, -1.0f};
    vec3 up = {.0f, 1.f, .0f};

    memcpy(cam->eye, eye, sizeof(eye));
    memcpy(cam->center, center, sizeof(center));
    memcpy(cam->up, up, sizeof(up));

    mat4x4_look_at(cam->v, cam->eye, cam->center, cam->up);
    mat4x4_perspective(cam->p, cam->fov, cam->aspect, cam->znear, cam->zfar);
}

void cam_rotate(struct Camera* cam, quat q) {
    mat4x4 rot;
    mat4x4_from_quat(rot, q);
    mat4x4_mul(cam->v, cam->v, rot);
}

void cam_translate(struct Camera* cam, vec3 t) {
    mat4x4_translate_in_place(cam->v, t[0], t[1], t[2]);
}
