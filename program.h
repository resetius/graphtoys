#pragma once

#include "glad/gl.h"

#include "linmath.h"

struct Program {
    GLuint program;
    void (*log)(const char* msg);
};

struct Program* prog_new();
void prog_free(struct Program* p);

int prog_add_vs(struct Program* p, const char* shader);
int prog_add_fs(struct Program* p, const char* shader);

int prog_link(struct Program* p);
int prog_use(struct Program* p);
int prog_validate(struct Program* p);

int prog_set_mat3x3(struct Program* p, const char* name, mat3x3 mat);
int prog_set_mat4x4(struct Program* p, const char* name, mat4x4 mat);
