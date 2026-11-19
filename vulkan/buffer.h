#pragma once

#include <render/buffer.h>

#include "vk.h"

struct BufferImpl {
    struct BufferBase base;
    VkBuffer buffer[10];       // buffer per swapchain image
    VkDeviceMemory memory[10]; // used for uniform buffers
    int n_buffers;
    VkDeviceSize size;
    VkMemoryPropertyFlags memory_flags; // of memory[], the same for all of them
};

struct Render;
struct BufferManager* buf_mgr_vulkan_new(struct Render* r);
