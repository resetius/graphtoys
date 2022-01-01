#include "render.h"

struct Program* rend_prog_new(struct Render* r) {
    return r->prog_new(r);
}

struct Mesh* rend_mesh_new(struct Render* r) {
    return r->mesh_new(r);
}

struct Texture* rend_tex_new(struct Render* r) {
    return r->tex_new(r);
}

struct Char* rend_char_new(struct Render* r, wchar_t ch, void* face) {
    return r->char_new(r, ch, face);
}

void rend_free(struct Render* r) {
    if (r) {
        r->free(r);
    }
}
