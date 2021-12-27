#pragma once

struct Font {
    void (*render)(struct Font* f, float x, float y, const char* string, float ratio);
};

struct Font* font_new();
void font_free(struct Font*);
