#pragma once

struct Char {
    wchar_t ch;
    int w;
    int h;
    int left;
    int top;
    int advance;

    void (*render)(struct Char* ch, int x, int y);
};
