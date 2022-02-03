#include "pipeline.h"

void pl_free(struct Pipeline* pl) {
    pl->free(pl);
}

void pl_storage_assign(struct Pipeline* p1, int storage_id, int buffer_id)
{
    p1->storage_assign(p1, storage_id, buffer_id);
}

void pl_uniform_assign(struct Pipeline* p1, int uniform_id, int buffer_id)
{
    p1->uniform_assign(p1, uniform_id, buffer_id);
}

int pl_buffer_assign(struct Pipeline* p1, int descriptor_id, int buffer_id)
{
    return p1->buffer_assign(p1, descriptor_id, buffer_id);
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
