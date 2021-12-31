#include "program.h"

void prog_free(struct Program* p) {
    p->free(p);
}

int prog_add_vs(struct Program* p, const char* shader) {
    return p->add_vs(p, shader);
}

int prog_add_fs(struct Program* p, const char* shader) {
    return p->add_fs(p, shader);
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

int prog_set_sub_fs(struct Program* p, const char* name) {
    return p->set_sub_fs(p, name);
}

int prog_attrib_location(struct Program* p, const char* name) {
    return p->attrib_location(p, name);
}

int prog_handle(struct Program* p) {
    return p->handle(p);
}
