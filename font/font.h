#pragma once

struct Font {
    void (*render)(struct Font* f, float x, float y, const char* string);
};

struct Font* font_new();
void font_free(struct Font*);
