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
    // uniform create per-image
    // dynamic create per-image?
    int (*create)(
        struct BufferManager* mgr,
        enum BufferType type,
        enum BufferMemoryType mem_type,
        const void* data,
        int size);

    void (*update)(
        struct BufferManager* mgr,
        int id,
        const void* data,
        int offset,
        int size);

    // for OpenGL update_sync == update
    // for Vulkan vkCmdUpdateBuffer is used with Barrier
    void (*update_sync)(
        struct BufferManager* mgr,
        int id,
        const void* data,
        int offset,
        int size,
        int flags // 0 for graphics queue, 1 for compute queue
        );

    void (*read)(
        struct BufferManager* mgr,
        int id,
        void* data,
        int offset,
        int size);

    void (*destroy)(struct BufferManager* mgr, int id);

    // internal handler
    void* (*get)(struct BufferManager* mgr, int id);
    void (*release)(struct BufferManager* mgr, void* buffer);
    struct BufferBase* (*acquire)(struct BufferManager* mgr, int buffer_size);

    void (*free)(struct BufferManager* mgr);
};

int buffer_create(
    struct BufferManager* b,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    const void* data,
    int size);

void buffer_update(
    struct BufferManager* b,
    int id,
    const void* data,
    int offset,
    int size);

// buffer_id
// local_buffer_id (buffer_id in pipeline)
// local_buffer_id = pipeline->assign(buffer_id, binding)
// pipeline->use_buffer(local_buffer_id)
// pipeline->swap_assignments(local_buffer_id1, local_buffer_id2)

struct BufferBase {
    int id;
    int valid;
    int size;
};

struct BufferManagerBase {
    struct BufferManager iface;

    int buffer_size;
    char* buffers;
    int n_buffers;
    int cap;
    int64_t total_memory;
};

void buffermanager_base_ctor(struct BufferManagerBase* bm, int buffer_size);
