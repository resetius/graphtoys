#pragma once

#include <lib/linmath.h>

struct Camera {
    vec3 eye;
    vec3 center;
    vec3 up;

    float fov;
    float aspect;
    float znear;
    float zfar;

    mat4x4 v;
    mat4x4 p;
};

void cam_init(struct Camera* cam);
void cam_rotate(struct Camera* cam, quat q);
void cam_translate(struct Camera* cam, vec3 translation);
