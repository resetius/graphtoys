#pragma once

struct DrawContext {
    float ratio;
};

struct Object {
    void (*draw)(struct Object*, struct DrawContext* );
    void (*free)(struct Object*);
};
