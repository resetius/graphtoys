#pragma once

void create_buffer(
    VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory);

void copy_buffer(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size);


void create_image(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage* image,
    VkDeviceMemory* imageMemory);
