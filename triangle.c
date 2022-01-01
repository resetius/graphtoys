#include <stdlib.h>
#include <stdio.h>

#include "glad/gl.h"
#include <GLFW/glfw3.h>

#include "triangle.h"
#include "linmath.h"

#include "triangle_vertex_shader.h"
#include "triangle_fragment_shader.h"

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

    GLuint ubo_binding;
    GLuint ubo_buffer;
    GLuint ubo_index;
};

void tr_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Triangle* tr = (struct Triangle*)obj;
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    mat4x4_rotate_Z(m, m, (float)glfwGetTime());
    mat4x4_ortho(p, -ctx->ratio, ctx->ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);

    glBindBuffer(GL_UNIFORM_BUFFER, tr->ubo_buffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(mat4x4), &mvp[0][0]);

    glUseProgram(tr->program);

//glUniformMatrix4fv(tr->mvp_location, 1, GL_FALSE, (const GLfloat*) &mvp);
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
    glShaderSource(tr->vertex_shader, 1, &triangle_vertex_shader, NULL);
    glCompileShader(tr->vertex_shader);

    int result;
    glGetShaderiv(tr->vertex_shader, GL_COMPILE_STATUS, &result);
    if (GL_FALSE == result) {
        // get error log
        int length = 0;
        puts("Shader compilation failed");
        glGetShaderiv(tr->vertex_shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char* msg = malloc(length+1);
            int written = 0;
            glGetShaderInfoLog(tr->vertex_shader, length, &written, msg);
            puts(msg);
            free(msg);
        }
        return 0;
    }

    tr->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(tr->fragment_shader, 1, &triangle_fragment_shader, NULL);
    glCompileShader(tr->fragment_shader);

    tr->program = glCreateProgram();
    glAttachShader(tr->program, tr->vertex_shader);
    glAttachShader(tr->program, tr->fragment_shader);
    glLinkProgram(tr->program);

    tr->ubo_index = glGetUniformBlockIndex(tr->program, "MatrixBlock");
    tr->ubo_binding = 0;
    glUniformBlockBinding(tr->program, tr->ubo_index, tr->ubo_binding);

    glGenBuffers(1, &tr->ubo_buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, tr->ubo_buffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(mat4x4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, tr->ubo_binding, tr->ubo_buffer);

    //tr->mvp_location = glGetUniformLocation(tr->program, "MVP");
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
