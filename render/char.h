#pragma once

struct Char {
    wchar_t ch;
    int w;
    int h;
    int left;
    int top;
    int advance;

    void* (*texture)(struct Char* ch);
    void (*free)(struct Char* ch);
};
