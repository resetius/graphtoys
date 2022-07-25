#include "program.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <glad/gl.h>

struct ProgramImpl {
    struct Program base;
    GLuint program;
    GLint major;
    GLint minor;
    void (*log)(const char* msg);
};

static void log_func(const char* msg) {
    puts(msg);
}

static void prog_log(struct ProgramImpl* p, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    p->log(buffer);
    va_end(args);
}

static void prog_free_(struct Program* p1) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
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

static int prog_add_(struct ProgramImpl* p, struct ShaderCode code, GLuint shader) {
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

static int prog_add_vs_(struct Program* p1, struct ShaderCode shader) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLuint shaderId = glCreateShader(GL_VERTEX_SHADER);
    return prog_add_(p, shader, shaderId);
}

static int prog_add_fs_(struct Program* p1, struct ShaderCode shader) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLuint shaderId = glCreateShader(GL_FRAGMENT_SHADER);
    return prog_add_(p, shader, shaderId);
}

static int prog_add_cs_(struct Program* p1, struct ShaderCode shader) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    if (p->major > 4 || (p->major == 4 && p->minor >= 3)) {
        GLuint shaderId = glCreateShader(GL_COMPUTE_SHADER);
        return prog_add_(p, shader, shaderId);
    } else {
        printf("Compute shader is not supported\n");
        exit(-1);
        return -1;
    }
}

static int prog_use_(struct Program* p1) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    glUseProgram(p->program);
    return 1;
}

static int prog_link_(struct Program* p1) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
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

static int prog_set_mat3x3_(struct Program* p1, const char* name, const mat3x3* mat) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniformMatrix3fv(location, 1, GL_FALSE, (const GLfloat*)mat);
    return 1;
}

static int prog_set_mat4x4_(struct Program* p1, const char* name, const mat4x4* mat) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, (const GLfloat*)mat);
    return 1;
}

static int prog_set_vec3_(struct Program* p1, const char* name, const vec3* vec) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniform3fv(location, 1, (const GLfloat*)vec);
    return 1;
}

static int prog_set_int_(struct Program*p1, const char* name, int* values, int n_values) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLint location = glGetUniformLocation(p->program, name);
    if (location < 0) {
        prog_log(p, "Unknown location: '%s'", name);
        return 0;
    }
    glUniform1iv(location, n_values, values);
    return 1;
}


static int prog_set_sub_fs_(struct Program* p1, const char* name) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    GLuint location = glGetSubroutineIndex(p->program, GL_FRAGMENT_SHADER, name);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &location);
    return 1;
}


static int prog_attrib_location_(struct Program* p1, const char* name) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    return glGetAttribLocation(p->program, name);
}

static int prog_handle_(struct Program* p1) {
    struct ProgramImpl* p = (struct ProgramImpl*)p1;
    return p->program;
}

struct Program* prog_opengl_new() {
    struct ProgramImpl* p = calloc(1, sizeof(struct ProgramImpl));
    struct Program base = {
        .free = prog_free_,
        .link = prog_link_,
        .use = prog_use_,
        .add_vs = prog_add_vs_,
        .add_fs = prog_add_fs_,
        .add_cs = prog_add_cs_,
        .set_mat3x3 = prog_set_mat3x3_,
        .set_mat4x4 = prog_set_mat4x4_,
        .set_vec3 = prog_set_vec3_,
        .set_int = prog_set_int_,
        .set_sub_fs = prog_set_sub_fs_,
        .attrib_location = prog_attrib_location_,
        .handle = prog_handle_
    };
    p->base = base;
    p->program = glCreateProgram();
    p->log = log_func;

    glGetIntegerv(GL_MAJOR_VERSION, &p->major);
    glGetIntegerv(GL_MINOR_VERSION, &p->minor);

    return (struct Program*)p;
}

void prog_free(struct Program* p) {
    p->free(p);
}

int prog_add_vs(struct Program* p, struct ShaderCode shader) {
    return p->add_vs(p, shader);
}

int prog_add_fs(struct Program* p, struct ShaderCode shader) {
    return p->add_fs(p, shader);
}

int prog_add_cs(struct Program* p, struct ShaderCode shader) {
    return p->add_cs(p, shader);
}

int prog_link(struct Program* p) {
    return p->link(p);
}

int prog_use(struct Program* p) {
    return p->use(p);
}

int prog_validate(struct Program* p) {
    return p->validate(p);
}

int prog_set_mat3x3(struct Program* p, const char* name, const mat3x3* mat) {
    return p->set_mat3x3(p, name, mat);
}

int prog_set_mat4x4(struct Program* p, const char* name, const mat4x4* mat) {
    return p->set_mat4x4(p, name, mat);
}

int prog_set_vec3(struct Program* p, const char* name, const vec3* vec) {
    return p->set_vec3(p, name, vec);
}

int prog_set_int(struct Program* p, const char* name, int* values, int n_values) {
    return p->set_int(p, name, values, n_values);
}

int prog_set_sub_fs(struct Program* p, const char* name) {
    return p->set_sub_fs(p, name);
}

int prog_attrib_location(struct Program* p, const char* name) {
    return p->attrib_location(p, name);
}

int prog_handle(struct Program* p) {
    return p->handle(p);
}
