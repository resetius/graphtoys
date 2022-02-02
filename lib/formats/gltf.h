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
    struct GltfPrimitive primitives[100];
    int n_primitives;
};

struct GltfNode {
    char name[256];
    int mesh;
    quat rotation;
};

struct GltfScene {
    char name[256];
    int nodes[1024];
    int n_nodes;
};

struct Gltf {
    struct GltfAccessor accessors[1024];
    int n_accessors;
    struct GltfScene scenes[1024];
    int n_scenes;
    struct GltfNode nodes[1024];
    int n_nodes;
    struct GltfMesh meshes[1024];
    int n_meshes;
    struct GltfBuffer buffers[1024];
    int n_buffers;
    struct GltfView views[1024];
    int n_views;
    int def;

    char fsbase[1024];
};

struct Gltf* gltf_load(const char* fn);
void gltf_destroy(struct Gltf*);
