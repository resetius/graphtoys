#pragma once

#include <stdint.h>

enum BufferType {
    BUFFER_ARRAY, // vao
    BUFFER_SHADER_STORAGE,
    BUFFER_UNIFORM,
    BUFFER_INDEX
};

enum BufferMemoryType {
    MEMORY_STATIC,
    MEMORY_DYNAMIC,
    MEMORY_DYNAMIC_COPY,
    MEMORY_DYNAMIC_READ
};

// Создаем все виды буферов (vertex, storage, uniform) ...
// Делаем привязку буфера к descriptor'у через pipeline
// Для vulkan внутри хранится descriptorset
// Для opengl binding id
// Идентификаторы должны храниться внутри pipeline, так как один буффер может
// использоваться в разных pipeline с разными binding_id/descriptor_set
// Перед использованием буфера шлем descriptorset в сommand buffer или делаем bind
struct BufferManager {
    const struct BufferManagerVtbl* vtbl;
};

struct BufferManagerVtbl {
    // uniform create per-image
    // dynamic create per-image?
    int (*create)(
        struct BufferManager* mgr,
        enum BufferType type,
        enum BufferMemoryType mem_type,
        const void* data,
        size_t size);

    void (*update)(
        struct BufferManager* mgr,
        int id,
        const void* data,
        size_t offset,
        size_t size);

    // for OpenGL update_sync == update
    // for Vulkan vkCmdUpdateBuffer is used with Barrier
    void (*update_sync)(
        struct BufferManager* mgr,
        int id,
        const void* data,
        size_t offset,
        size_t size,
        int flags // 0 for graphics queue, 1 for compute queue
        );

    void (*read)(
        struct BufferManager* mgr,
        int id,
        void* data,
        size_t offset,
        size_t size);

    // internal handler
    void (*release)(struct BufferManager* mgr, void* buffer);
};

#define buffer_create(b, type, mem_type, data, size) ((b)->vtbl->create((b), (type), (mem_type), (data), (size)))
#define buffer_update(b, id, data, offset, size) ((b)->vtbl->update((b), (id), (data), (offset), (size)))
#define buffer_update_sync(b, id, data, offset, size, flags) ((b)->vtbl->update_sync((b), (id), (data), (offset), (size), (flags)))
#define buffer_read(b, id, data, offset, size) ((b)->vtbl->read((b), (id), (data), (offset), (size)))

// Not virtual: the same for all backends (render/buffer.c).
void buffer_destroy(struct BufferManager* mgr, int id);
void* buffer_get(struct BufferManager* mgr, int id);
struct BufferBase* buffer_acquire(struct BufferManager* mgr, size_t buffer_size);
void buffer_mgr_free(struct BufferManager* mgr);

// buffer_id
// local_buffer_id (buffer_id in pipeline)
// local_buffer_id = pipeline->assign(buffer_id, binding)
// pipeline->use_buffer(local_buffer_id)
// pipeline->swap_assignments(local_buffer_id1, local_buffer_id2)

struct BufferBase {
    int id;
    int valid;
    size_t size;
};

struct BufferManagerBase {
    struct BufferManager iface;

    size_t buffer_size;
    char* buffers;
    int n_buffers;
    int cap;
    int64_t total_memory;
};

void buffermanager_base_ctor(struct BufferManagerBase* bm, size_t buffer_size);
