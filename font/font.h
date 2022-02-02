#pragma once

#include <stdint.h>

#include <render/render.h>

struct Font;

struct BufferPair;

struct Label {
    struct Font* f;
    uint32_t* text;
    struct BufferPair* buf;
    int len;
    int cap;

    int x;
    int y;
    int w;
    int h;
    int id;

    int dirty;
};

struct Font* font_new(struct Render* r,
                      int char_width,
                      int char_height,
                      int device_w,
                      int device_h);
void font_free(struct Font*);

struct Label* label_new(struct Font* f);
void label_set_text(struct Label* l, const char* s);
void label_set_vtext(struct Label* l, const char* s, ...);
void label_set_pos(struct Label* l, int x, int y);
void label_set_screen(struct Label* l, int w, int h);
void label_render(struct Label* l);
void label_free(struct Label* l);
