#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "torus.h"
#include "linmath.h"
#include "triangle_fragment_shader.h"
#include "torus_vertex_shader.h"

// x = (R+r cos(psi)) cos phi
// y = (R+r cos(psi)) sin phi
// z = r sin phi
// psi in [0,2pi)
// phi in [-pi,pi)

struct Vertex {
    vec3 pos;
    vec3 col;
};

struct Torus {
    struct Object base;
    GLuint vertex_buffer;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;

    GLint mvp_location;
    GLint vpos_location;
    GLint vcol_location;

    GLuint vertex_array;

    struct Vertex* vertices;
    int nvertices;
};

static struct Vertex* init (int* nvertices) {
    float R = 0.5;
    float r = 0.25;
    int i,j,k;
#define N 50
#define M 50
    int n = N;
    int m = M;
    vec3 A[N][M];
    struct Vertex* vertices = calloc(n*m*3*2, sizeof(struct Vertex));

    for (i = 0; i < n; i++) {
        float phi = 2.f*M_PI*i/n;
        for (j = 0; j < m; j++) {
            float psi = 2.f*M_PI*j/m-M_PI/2.;

            float x = (R+r*cosf(psi))*cosf(phi);
            float y = (R+r*cosf(psi))*sinf(phi);
            float z = r*sinf(psi);
            vec3 v = {x,y,z};

            memcpy(A[i][j], v, sizeof(v));
        }
    }

    k = 0;
    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            /*
            {i,j},{i,j+1}
            {i+1,j}
                    {i,j+1}
            {i+1,j},{i+1,j+1}
            */

            memcpy(vertices[k++].pos, A[i][j], sizeof(vec3));
            memcpy(vertices[k++].pos, A[i][(j+1)%m], sizeof(vec3));
            memcpy(vertices[k++].pos, A[(i+1)%n][j], sizeof(vec3));

            memcpy(vertices[k++].pos, A[i][(j+1)%m], sizeof(vec3));
            memcpy(vertices[k++].pos, A[(i+1)%n][j], sizeof(vec3));
            memcpy(vertices[k++].pos, A[(i+1)%n][(j+1)%m], sizeof(vec3));
        }
    }

    for (i = 0; i < k; i++) {
        vec3 col = {
            rand()/(float)RAND_MAX,
            rand()/(float)RAND_MAX,
            rand()/(float)RAND_MAX
        };
        memcpy(vertices[i].col, col, sizeof(col));
        //vertices[i].col[0] = fmax(0, vertices[i].pos[2]);
    }

    printf("nvertices %d\n", k);
#undef N
#undef M
    *nvertices = k;
    return vertices;
}

static void t_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Torus* t = (struct Torus*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_X(m, m, (float)glfwGetTime());
    mat4x4_rotate_Y(m, m, (float)glfwGetTime());
    mat4x4_rotate_Z(m, m, (float)glfwGetTime());
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    glUseProgram(t->program);
    glUniformMatrix4fv(t->mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);
    glBindVertexArray(t->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, t->nvertices);
}

static void t_free(struct Object* obj) {
    struct Torus* t = (struct Torus*)obj;
// TODO: free opengl resources
    free(t->vertices);
    free(t);
}

struct Object* CreateTorus() {
    struct Torus* t = calloc(1, sizeof(struct Torus));
    t->base.draw = t_draw;
    t->base.free = t_free;
    t->vertices = init(&t->nvertices);

    glGenBuffers(1, &t->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, t->nvertices*sizeof(struct Vertex),
                 t->vertices, GL_STATIC_DRAW);

    t->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(t->vertex_shader, 1, &torus_vertex_shader, NULL);
    glCompileShader(t->vertex_shader);

    t->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(t->fragment_shader, 1, &triangle_fragment_shader, NULL);
    glCompileShader(t->fragment_shader);

    t->program = glCreateProgram();
    glAttachShader(t->program, t->vertex_shader);
    glAttachShader(t->program, t->fragment_shader);
    glLinkProgram(t->program);

    t->mvp_location = glGetUniformLocation(t->program, "MVP");
    t->vpos_location = glGetAttribLocation(t->program, "vPos");
    t->vcol_location = glGetAttribLocation(t->program, "vCol");

    glGenVertexArrays(1, &t->vertex_array);
    glBindVertexArray(t->vertex_array);
    glEnableVertexAttribArray(t->vpos_location);
    glVertexAttribPointer(
        t->vpos_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(struct Vertex), (void*) offsetof(struct Vertex, pos));
    glEnableVertexAttribArray(t->vcol_location);
    glVertexAttribPointer(
        t->vcol_location, 3, GL_FLOAT, GL_FALSE,
        sizeof(struct Vertex), (void*) offsetof(struct Vertex, col));

    return (struct Object*)t;
}
