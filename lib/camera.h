#pragma once

#include <lib/linmath.h>
#include <lib/event.h>

// TODO: orthographic camera
struct Camera {
    vec3 eye;
    vec3 center;
    vec3 up;

    float fov;
    float aspect;
    float znear;
    float zfar;
};

void cam_init(struct Camera* cam);
void cam_update(mat4x4 v, mat4x4 p, struct Camera* cam);

void cam_rotate(struct Camera* cam, quat q);
void cam_translate(struct Camera* cam, vec3 translation);

struct CameraEventConsumer {
    struct EventConsumer base;
    struct Camera* cam;
};

void cam_event_consumer_init(struct CameraEventConsumer* cons, struct Camera* cam);
