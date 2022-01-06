#include "ref.h"

void ref(struct Ref* r) {
    atomic_fetch_add(&r->counter, 1);
}

void unref(struct Ref* r) {
    if (atomic_fetch_sub(&r->counter, 1) == 1) {
        r->free(r);
    }
}
