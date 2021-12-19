#pragma once

#include "glad/gl.h"

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

int prog_bind_attr(struct Program* p, GLuint location, const char* name);
int prog_set_uniform(struct Program* p, const char* name);
