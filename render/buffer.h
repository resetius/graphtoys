#pragma once

#include <stdint.h>

enum BufferType {
    BUFFER_ARRAY, // vao
    BUFFER_SHADER_STORAGE,
    BUFFER_UNIFORM,
};

enum BufferMemoryType {
    MEMORY_STATIC,
    MEMORY_DYNAMIC,
    MEMORY_DYNAMIC_COPY
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

    void (*destroy)(struct BufferManager* mgr, int id);

    // internal handler
    void* (*get)(struct BufferManager* mgr, int id);
    void (*release)(struct BufferManager* mgr, void* buffer);

    void (*free)(struct BufferManager* mgr);
};

// buffer_id
// local_buffer_id (buffer_id in pipeline)
// local_buffer_id = pipeline->assign(buffer_id, binding)
// pipeline->use_buffer(local_buffer_id)
// pipeline->swap_assignments(local_buffer_id1, local_buffer_id2)

struct BufferBase {
    int id;
    int valid;
};

struct BufferManagerBase {
    struct BufferManager iface;

    int buffer_size;
    char* buffers;
    int n_buffers;
    int cap;
};

struct BufferBase* buffer_acquire_(struct BufferManagerBase* mgr);
void buffer_release_(struct BufferManager* mgr, int id);
void* buffer_get_(struct BufferManager* mgr, int id);
void buffer_mgr_free_(struct BufferManager* mgr);
