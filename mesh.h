#pragma once

#include "linmath.h"
#include "program.h"

struct Vertex {
    vec3 pos;
    vec3 norm;
    vec3 col;
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

void mesh_free(struct Mesh*);
void mesh_render(struct Mesh*);
