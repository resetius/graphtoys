#include "program.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <glad/gl.h>

struct Program {
    GLuint program;
    GLint major;
    GLint minor;
    void (*log)(const char* msg);
};

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
    free(shaders);

    glDeleteProgram(p->program);
    free(p);
}

static int prog_add_(struct Program* p, struct ShaderCode code, GLuint shader) {
    int result;

    if (code.glsl) {
        glShaderSource(shader, 1, &code.glsl, NULL);
        glCompileShader(shader);
    } else {
        glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V,
                       code.spir_v, code.size);
        glSpecializeShader(shader, "main", 0, NULL, NULL);
    }

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

int prog_add_vs(struct Program* p, struct ShaderCode shader) {
    GLuint shaderId = glCreateShader(GL_VERTEX_SHADER);
    return prog_add_(p, shader, shaderId);
}

int prog_add_fs(struct Program* p, struct ShaderCode shader) {
    GLuint shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    return prog_add_(p, shader, shaderId);
}

int prog_add_cs(struct Program* p, struct ShaderCode shader) {
    if (p->major > 4 || (p->major == 4 && p->minor >= 3)) {
        GLuint shaderId = glCreateShader(GL_COMPUTE_SHADER);
        return prog_add_(p, shader, shaderId);
    } else {
        printf("Compute shader is not supported\n");
        exit(-1);
        return -1;
    }
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

int prog_set_int(struct Program* p, const char* name, int* values, int n_values) {
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniform1iv(location, n_values, values);
    return 1;
}


int prog_set_sub_fs(struct Program* p, const char* name) {
    GLuint location = glGetSubroutineIndex(p->program, GL_FRAGMENT_SHADER, name);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &location);
    return 1;
}


int prog_attrib_location(struct Program* p, const char* name) {
    return glGetAttribLocation(p->program, name);
}

int prog_handle(struct Program* p) {
    return p->program;
}

struct Program* prog_opengl_new(void) {
    struct Program* p = calloc(1, sizeof(struct Program));
    p->program = glCreateProgram();
    p->log = log_func;

    glGetIntegerv(GL_MAJOR_VERSION, &p->major);
    glGetIntegerv(GL_MINOR_VERSION, &p->minor);

    return p;
}
