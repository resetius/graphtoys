#pragma once

#include "vk.h"

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

VkImageView create_image_view(
    VkDevice logDev, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

void transition_image_layout(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

void copy_buffer_to_image(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
