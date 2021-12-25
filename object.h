#pragma once

struct DrawContext {
    float ratio;
};

struct Object {
    void (*draw)(struct Object*, struct DrawContext* );
    void (*free)(struct Object*);
    void (*move_left)(struct Object*);
    void (*move_right)(struct Object*);
    void (*move_up)(struct Object*);
    void (*move_down)(struct Object*);
    void (*zoom_in)(struct Object*);
    void (*zoom_out)(struct Object*);
};

struct ObjectVec {
    struct Object** objs;
    int cap;
    int size;
};

void ovec_free(struct ObjectVec* );
void ovec_add(struct ObjectVec*, struct Object*);
struct Object* ovec_get(struct ObjectVec*, int index);
