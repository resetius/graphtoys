#include <stdlib.h>
#include <stdio.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "triangle.h"
#include "linmath.h"

typedef struct Vertex
{
    vec2 pos;
    vec3 col;
} Vertex;

static const Vertex vertices[3] =
{
    { { -0.6f, -0.4f }, { 1.f, 0.f, 0.f } },
    { {  0.6f, -0.4f }, { 0.f, 1.f, 0.f } },
    { {   0.f,  0.6f }, { 0.f, 0.f, 1.f } }
};

struct Triangle {
    struct Object base;
    GLuint vertex_buffer;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;

    GLint mvp_location;
    GLint vpos_location;
    GLint vcol_location;

    GLuint vertex_array;
};

static const char* vertex_shader_text =
"#version 330\n"
"uniform mat4 MVP;\n"
"in vec3 vCol;\n"
"in vec2 vPos;\n"
"out vec3 color;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    color = vCol;\n"
"}\n";

static const char* fragment_shader_text =
"#version 330\n"
"in vec3 color;\n"
"out vec4 fragment;\n"
"void main()\n"
"{\n"
"    fragment = vec4(color, 1.0);\n"
"}\n";

void tr_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Triangle* tr = (struct Triangle*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, (float)glfwGetTime());
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    glUseProgram(tr->program);
    glUniformMatrix4fv(tr->mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);
    glBindVertexArray(tr->vertex_array);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void tr_free(struct Object* obj) {
    free(obj);
}

struct Object* CreateTriangle() {
    struct Triangle* tr = calloc(1, sizeof(struct Triangle));
    tr->base.draw = tr_draw;
    tr->base.free = tr_free;

    glGenBuffers(1, &tr->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, tr->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    tr->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(tr->vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShader(tr->vertex_shader);

    tr->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(tr->fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShader(tr->fragment_shader);

    tr->program = glCreateProgram();
    glAttachShader(tr->program, tr->vertex_shader);
    glAttachShader(tr->program, tr->fragment_shader);
    glLinkProgram(tr->program);

    tr->mvp_location = glGetUniformLocation(tr->program, "MVP");
    tr->vpos_location = glGetAttribLocation(tr->program, "vPos");
    tr->vcol_location = glGetAttribLocation(tr->program, "vCol");

    glGenVertexArrays(1, &tr->vertex_array);
    glBindVertexArray(tr->vertex_array);
    glEnableVertexAttribArray(tr->vpos_location);
    glVertexAttribPointer(tr->vpos_location, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, pos));
    glEnableVertexAttribArray(tr->vcol_location);
    glVertexAttribPointer(tr->vcol_location, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*) offsetof(Vertex, col));

    return (struct Object*)tr;
}
