#include "pipeline.h"

void pl_free(struct Pipeline* pl) {
    pl->free(pl);
}

void pl_uniform_update(struct Pipeline* pl, int id, const void* data, int offset, int size) {
    pl->uniform_update(pl, id, data, offset, size);
}

void pl_buffer_update(struct Pipeline* pl, int od, const void* data, int offset, int size) {
    pl->buffer_update(pl, od, data, offset, size);
}

int pl_buffer_create(struct Pipeline* pl, enum BufferType type, enum BufferMemoryType mtype,
                      int binding, const void* data, int size) {
    return pl->buffer_create(pl, type, mtype, binding, data, size);
}

void pl_use_texture(struct Pipeline* pl, void* texture) {
    pl->use_texture(pl, texture);
}

void pl_start(struct Pipeline* pl) {
    pl->start(pl);
}

void pl_draw(struct Pipeline* pl, int buffer_id) {
    pl->draw(pl, buffer_id);
}
