#pragma once

#include <lib/linmath.h>
#include <stdint.h>

struct GltfAccessor {
    char name[256];
    int64_t offset;
    int view;
    int component_type;
    int count;
    int components;
    vec4 max;
    vec4 min;
};

struct GltfBuffer {
    char* data;
    int64_t size;
    int buffer;
};

struct GltfView {
    char name[256];
    char* data;
    int buffer;
    int stride;
    int64_t size;
};

struct GltfMaterial {
    char name[256];
    // TODO
};

struct GltfPrimitive {
    // attributes
    int normal;
    int position;
    int texcoord[10];
    int n_texcoords;

    int indices;
    int material;
};

struct GltfMesh {
    char name[256];
    struct GltfPrimitive* primitives;
    int cap_primitives;
    int n_primitives;
};

struct GltfNode {
    char name[256];
    int mesh;
    int camera;
    quat rotation;
    vec3 scale;
    vec3 translation;
    mat4x4 matrix;
    mat4x4 norm_matrix;
};

struct GltfScene {
    char name[256];
    int nodes[1024];
    int n_nodes;
};

struct GltfCameraPerspective {
    double aspect;
    double yfov;
    double zfar;
    double znear;
};

struct GltfCameraOrthographic {
    double xmag;
    double ymag;
    double zfar;
    double znear;
};

struct GltfCamera {
    char name[256];
    int is_perspective;
    struct GltfCameraPerspective perspective;
    struct GltfCameraOrthographic orthographic;
};

struct Gltf {
    struct GltfAccessor* accessors;
    int cap_accessors;
    int n_accessors;
    struct GltfScene* scenes;
    int cap_scenes;
    int n_scenes;
    struct GltfNode* nodes;
    int cap_nodes;
    int n_nodes;
    struct GltfMesh* meshes;
    int cap_meshes;
    int n_meshes;
    struct GltfBuffer* buffers;
    int cap_buffers;
    int n_buffers;
    struct GltfView* views;
    int cap_views;
    int n_views;
    struct GltfCamera* cameras;
    int cap_cameras;
    int n_cameras;

    int def;

    char fsbase[1024];
};

struct Gltf* gltf_load(const char* fn);
void gltf_destroy(struct Gltf*);
