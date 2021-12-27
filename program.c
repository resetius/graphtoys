#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "program.h"

static void log_func(const char* msg) {
    puts(msg);
}

static void prog_log(struct Program* p, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    p->log(buffer);
    va_end(args);
}

struct Program* prog_new() {
    struct Program* p = calloc(1, sizeof(struct Program));
    p->program = glCreateProgram();
    p->log = log_func;
    return p;
}

void prog_free(struct Program* p) {
    GLint numShaders = 0;
    GLuint *shaders;
    int i;

    if (!p->program) {
        return;
    }

    glGetProgramiv(p->program, GL_ATTACHED_SHADERS, &numShaders);
    shaders = malloc(numShaders*sizeof(GLint));
    glGetAttachedShaders(p->program, numShaders, NULL, shaders);

    for (i = 0; i < numShaders; i++) {
        glDeleteShader(shaders[i]);
    }

    glDeleteProgram(p->program);
    free(shaders);
}

static int prog_add(struct Program* p, const char* shaderText, GLuint shader) {
    int result;
    glShaderSource(shader, 1, &shaderText, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (GL_FALSE == result) {
        // get error log
        int length = 0;
        p->log("Shader compilation failed");
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char* msg = malloc(length+1);
            int written = 0;
            glGetShaderInfoLog(shader, length, &written, msg);
            p->log(msg);
            free(msg);
        }
        return 0;
    }
    glAttachShader(p->program, shader);
    return 1;
}

int prog_add_vs(struct Program* p, const char* shader) {
    GLuint shaderId = glCreateShader(GL_VERTEX_SHADER);
    return prog_add(p, shader, shaderId);
}

int prog_add_fs(struct Program* p, const char* shader) {
    GLuint shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    return prog_add(p, shader, shaderId);
}

int prog_use(struct Program* p) {
    glUseProgram(p->program);
    return 1;
}

int prog_link(struct Program* p) {
    int result;
    glLinkProgram(p->program);

    glGetProgramiv(p->program, GL_LINK_STATUS, &result);
    if (GL_FALSE == result) {
        // get error log
        int length = 0;
        p->log("Program link failed");
        glGetProgramiv(p->program, GL_INFO_LOG_LENGTH, &length);
        if (length > 0) {
            char* msg = malloc(length+1);
            int written = 0;
            glGetProgramInfoLog(p->program, length, &written, msg);
            p->log(msg);
            free(msg);
        }
        return 0;
    }

    return 1;
}

int prog_set_mat3x3(struct Program* p, const char* name, const mat3x3* mat) {
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniformMatrix3fv(location, 1, GL_FALSE, (const GLfloat*)mat);
    return 1;
}

int prog_set_mat4x4(struct Program* p, const char* name, const mat4x4* mat) {
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, (const GLfloat*)mat);
    return 1;
}

int prog_set_vec3(struct Program* p, const char* name, const vec3* vec) {
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniform3fv(location, 1, (const GLfloat*)vec);
    return 1;
}

int prog_set_sub_fs(struct Program* p, const char* name) {
    GLuint location = glGetSubroutineIndex(p->program, GL_FRAGMENT_SHADER, name);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &location);
    return 1;
}
