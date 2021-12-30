#pragma once

#include <stdatomic.h>

struct Ref {
    atomic_int counter;
    void (*free)(struct Ref* r);
};

void ref(struct Ref*);
void unref(struct Ref*);
