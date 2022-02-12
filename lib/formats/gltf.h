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

struct GltfTextureInfo {
    int index;
    int tex_coord; // 0
};

struct GltfMetallicRoughness {
    vec4 base_color_factor; // [1,1,1,1]
    struct GltfTextureInfo base_color_texture;
    int has_base_color_texture; // 0
    float metallic_factor; // 1
    float roughness_factor; // 1
    struct GltfTextureInfo metallic_roughness_texture;
    int has_metallic_roughness_texture; // 0
};

struct GltfNormalTextureInfo {
    int index;
    int tex_coord; // 0
    float scale; // 1
};

struct GltfOcclusionTextureInfo {
    int index;
    int tex_coord; // 0
    float strength; // 1
};

struct GltfMaterial {
    char name[256];

    struct GltfMetallicRoughness pbr_metallic_roughness;
    int has_pbr_metallic_roughness; // 0

    struct GltfNormalTextureInfo normal_texture;
    int has_normal_texture; // 0

    struct GltfOcclusionTextureInfo occlusion_texture;
    int has_occlusion_texture; // 0

    struct GltfTextureInfo emissive_texture;
    int has_emissive_texture; // 0
    vec3 emissive_factor; // [0,0,0]
    int alpha_mode; // OPAQUE
    float alpha_cutoff; // 0.5
    int double_sided; // 0
};

struct GltfPrimitive {
    // attributes
    int normal;
    int position;
    int texcoord[10];
    int n_texcoords;
    int tangent;

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

struct GltfImage {
    char name[256];
    void* texture;
};

struct GltfTexture {
    int source;
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
    struct GltfImage* images;
    int cap_images;
    int n_images;
    struct GltfTexture* textures;
    int cap_textures;
    int n_textures;
    struct GltfMaterial* materials;
    int cap_materials;
    int n_materials;

    int def;

    char fsbase[1024];
};

struct Gltf* gltf_alloc();
void gltf_free(struct Gltf*);

void gltf_ctor(struct Gltf*, const char* fn);
void gltf_dtor(struct Gltf*);
