#include <stdlib.h>
#include <stdio.h>

#include "program.h"

static void log_func(const char* msg) {
    puts(msg);
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
