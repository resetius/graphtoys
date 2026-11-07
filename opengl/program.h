#pragma once

#include <lib/linmath.h>

#include <render/shader.h>

struct Program;

void prog_free(struct Program* p);

int prog_add_vs(struct Program* p, struct ShaderCode shader);
int prog_add_fs(struct Program* p, struct ShaderCode shader);
int prog_add_cs(struct Program* p, struct ShaderCode shader);

int prog_link(struct Program* p);
int prog_use(struct Program* p);

int prog_set_mat3x3(struct Program* p, const char* name, const mat3x3* mat);
int prog_set_mat4x4(struct Program* p, const char* name, const mat4x4* mat);

int prog_set_vec3(struct Program* p, const char* name, const vec3* vec);
int prog_set_int(struct Program*, const char* name, int* values, int n_values);

int prog_set_sub_fs(struct Program* p, const char* name);

int prog_attrib_location(struct Program* p, const char* name);
int prog_handle(struct Program* p);

struct Program* prog_opengl_new(void);
