#include <stdlib.h>
#include "object.h"

void ovec_free(struct ObjectVec* v) {
    free(v->objs);
}

void ovec_add(struct ObjectVec* v, struct Object* o) {
    if (v->cap <= v->size) {
        v->cap = (1+v->cap)*2;
        v->objs = realloc(v->objs, v->cap*sizeof(struct Object*));
    }
    v->objs[v->size++] = o;
}

struct Object* ovec_get(struct ObjectVec* v, int index) {
    return v->objs[index];
}
