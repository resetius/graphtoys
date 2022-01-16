#pragma once

#include <render/buffer.h>

#include <vulkan/vulkan.h>

struct BufferImpl {
    struct BufferBase base;
    VkBuffer buffer[10];       // buffer per swapchain image
    VkDeviceMemory memory[10]; // used for uniform buffers
    int n_buffers;
    VkDeviceSize size;
};

struct Render;
struct BufferManager* buf_mgr_vulkan_new(struct Render* r);
