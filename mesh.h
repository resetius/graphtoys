#pragma once

#include "linmath.h"
#include "program.h"

struct Vertex {
    vec3 pos;
    vec3 norm;
    vec3 col;
};

struct Vertex1 {
    vec3 pos;
};

struct TexVertex1 {
    vec3 pos;
    vec2 tpos;
};

struct Mesh {
    int nvertices;
    GLuint buffer;
    GLuint vao;
};

struct Mesh* mesh_new(
    struct Program* p,
    struct Vertex* vertices,
    int nvertices,
    const char* posName,
    const char* colName,
    const char* normName);

struct Mesh* mesh1_new(
    struct Vertex1* vertices,
    int nvertices,
    int vpos_location);

struct Mesh* mesh_tex1_new(
    struct TexVertex1* vertices,
    int nvertices,
    int vpos_location,
    int tpos_tlocation);

void mesh_free(struct Mesh*);
void mesh_render(struct Mesh*);
