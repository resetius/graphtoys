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
    quat_mul_vec3(cam->center, cam->center, q);
}

void cam_translate(struct Camera* cam, vec3 t) {
    memcpy(cam->eye, t, sizeof(cam->eye));
}
