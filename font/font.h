#pragma once

#include <render/render.h>

struct Font;

struct Label {
    struct Font* f;
    char* text;
    int cap;
    int x;
    int y;
    int w;
    int h;
    int id;
};

struct Font* font_new(struct Render* r);
void font_free(struct Font*);

struct Label* label_new(struct Font* f);
void label_set_text(struct Label* l, const char* s);
void label_set_vtext(struct Label* l, const char* s, ...);
void label_set_pos(struct Label* l, int x, int y);
void label_set_screen(struct Label* l, int w, int h);
void label_render(struct Label* l);
void label_free(struct Label* l);
