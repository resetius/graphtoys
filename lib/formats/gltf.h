#pragma once

#include <lib/linmath.h>
#include <stdint.h>

struct GltfAccessor {
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
    struct GltfAccessor accessors[100];
    int n_accessors;
    struct GltfScene scenes[100];
    int n_scenes;
    struct GltfNode nodes[100];
    int n_nodes;
    struct GltfMesh meshes[100];
    int n_meshes;
    struct GltfBuffer buffers[100];
    int n_buffers;
    struct GltfBuffer views[100];
    int n_views;
    int def;

    char fsbase[1024];
};

void gltf_load(struct Gltf*, const char* fn);
void gltf_destroy(struct Gltf*);
