#pragma once

struct DrawContext {
    float ratio;
    float time;
    int w;
    int h;
};

struct Object {
    const struct ObjectVtbl* vtbl;
};

struct ObjectVtbl {
    void (*draw)(struct Object*, struct DrawContext* );
    void (*free)(struct Object*);
    // optional, may be NULL
    void (*move_left)(struct Object*, int mods);
    void (*move_right)(struct Object*, int mods);
    void (*move_up)(struct Object*, int mods);
    void (*move_down)(struct Object*, int mods);
    void (*zoom_in)(struct Object*, int mods);
    void (*zoom_out)(struct Object*, int mods);
    void (*mode)(struct Object*);
};

#define obj_draw(o, ctx) ((o)->vtbl->draw((o), (ctx)))
#define obj_free(o) ((o)->vtbl->free(o))
#define obj_move_left(o, mods) ((o)->vtbl->move_left ? (o)->vtbl->move_left((o), (mods)) : (void)0)
#define obj_move_right(o, mods) ((o)->vtbl->move_right ? (o)->vtbl->move_right((o), (mods)) : (void)0)
#define obj_move_up(o, mods) ((o)->vtbl->move_up ? (o)->vtbl->move_up((o), (mods)) : (void)0)
#define obj_move_down(o, mods) ((o)->vtbl->move_down ? (o)->vtbl->move_down((o), (mods)) : (void)0)
#define obj_zoom_in(o, mods) ((o)->vtbl->zoom_in ? (o)->vtbl->zoom_in((o), (mods)) : (void)0)
#define obj_zoom_out(o, mods) ((o)->vtbl->zoom_out ? (o)->vtbl->zoom_out((o), (mods)) : (void)0)
#define obj_mode(o) ((o)->vtbl->mode ? (o)->vtbl->mode(o) : (void)0)

struct ObjectVec {
    struct Object** objs;
    int cap;
    int size;
};

void ovec_free(struct ObjectVec* );
void ovec_add(struct ObjectVec*, struct Object*);
struct Object* ovec_get(struct ObjectVec*, int index);
