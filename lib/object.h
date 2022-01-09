#pragma once

struct DrawContext {
    float ratio;
    float time;
    int w;
    int h;
};

struct Object {
    void (*draw)(struct Object*, struct DrawContext* );
    void (*free)(struct Object*);
    void (*move_left)(struct Object*, int mods);
    void (*move_right)(struct Object*, int mods);
    void (*move_up)(struct Object*, int mods);
    void (*move_down)(struct Object*, int mods);
    void (*zoom_in)(struct Object*, int mods);
    void (*zoom_out)(struct Object*, int mods);
    void (*mode)(struct Object*);
};

struct ObjectVec {
    struct Object** objs;
    int cap;
    int size;
};

void ovec_free(struct ObjectVec* );
void ovec_add(struct ObjectVec*, struct Object*);
struct Object* ovec_get(struct ObjectVec*, int index);
