#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include "glad/gl.h"

#include "mesh.h"

struct Mesh* mesh_new(
    struct Program* p,
    struct Vertex* vertices,
    int nvertices,
    const char* posName,
    const char* colName,
    const char* normName)
{
    struct Mesh* m = calloc(1, sizeof(struct Mesh));

    GLint vpos_location = glGetAttribLocation(p->program, posName);
    GLint vcol_location = glGetAttribLocation(p->program, colName);
    GLint vnorm_location = glGetAttribLocation(p->program, normName);

    m->nvertices = nvertices;
    glGenBuffers(1, &m->buffer);
    glBindBuffer(GL_ARRAY_BUFFER, m->buffer);
    glBufferData(GL_ARRAY_BUFFER, m->nvertices*sizeof(struct Vertex),
                 vertices, GL_STATIC_DRAW);

    glGenVertexArrays(1, &m->vao);
    glBindVertexArray(m->vao);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(
        vpos_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(struct Vertex), (void*) offsetof(struct Vertex, pos));

    glEnableVertexAttribArray(vcol_location);
    glVertexAttribPointer(
        vcol_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(struct Vertex), (void*) offsetof(struct Vertex, col));

    glEnableVertexAttribArray(vnorm_location);
    glVertexAttribPointer(
        vnorm_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(struct Vertex), (void*) offsetof(struct Vertex, norm));

    return m;
}

void mesh_free(struct Mesh* m)
{
    glDeleteBuffers(1, &m->buffer);
    glDeleteVertexArrays(1, &m->vao);
    free(m);
}

void mesh_render(struct Mesh* m)
{
    glBindVertexArray(m->vao);
    glDrawArrays(GL_TRIANGLES, 0, m->nvertices);
}
