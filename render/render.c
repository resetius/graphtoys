#include "render.h"

struct Texture* rend_tex_new(struct Render* r, void* data, enum TexType tex_type) {
    return r->tex_new(r, data, tex_type);
}

struct Char* rend_char_new(struct Render* r, wchar_t ch, void* face) {
    return r->char_new(r, ch, face);
}

void rend_free(struct Render* r) {
    if (r) {
        r->free(r);
    }
}
