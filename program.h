#pragma once

enum ShaderType {
    VERTEX,
    FRAGMENT
};

struct Program {
    GLuint program;
};

struct Program* prog_new();
void prog_free(struct Program* p);

void prog_add(struct Program* p, const char* shader, enum ShaderType type);
void prog_link(struct Program* p);
void prog_use(struct Program* p);
void prog_validate(struct Program* p);

void prog_bind_attr(struct Program* p, GLuint location, const char* name);
void prog_set_uniform(struct Program* p, const char* name);
