#pragma once

struct Char {
    const struct CharVtbl* vtbl;
    wchar_t ch;
    int w;
    int h;
    int left;
    int top;
    int advance;
};

struct CharVtbl {
    void* (*texture)(struct Char* ch);
    void (*free)(struct Char* ch);
};

#define char_texture(c) ((c)->vtbl->texture(c))
#define char_free(c) ((c)->vtbl->free(c))
