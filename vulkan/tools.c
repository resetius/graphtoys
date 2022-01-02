#include <stdio.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>

#include "tools.h"

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags &
             properties)
            == properties)
        {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type\n");
    exit(-1);
    return 0;
}

void create_buffer(
    VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer* buffer,
    VkDeviceMemory* bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    if (vkCreateBuffer(logicalDevice, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        exit(-1);
    }

    VkMemoryRequirements memrequirements;
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memrequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memrequirements.size,
        .memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memrequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate vertex buffer memory\n");
        exit(-1);
    }

    vkBindBufferMemory(logicalDevice, *buffer, *bufferMemory, 0);
}

static VkCommandBuffer beginSingleTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = commandPool,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void endSingleTimeCommands(VkDevice logicalDevice, VkQueue g_queue, VkCommandBuffer commandBuffer, VkCommandPool commandPool)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    vkQueueSubmit(g_queue, 1, &submitInfo, VK_NULL_HANDLE);

    vkQueueWaitIdle(g_queue);

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void copy_buffer(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkBuffer srcBuffer,
    VkBuffer dstBuffer,
    VkDeviceSize size)
{
    // Create Command Pool
    VkCommandPool commandPool;

    VkCommandPoolCreateInfo cpInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = graphics_family,
        .flags = 0
    };

    if (vkCreateCommandPool(logicalDevice, &cpInfo, NULL, &commandPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create command pool\n");
        exit(-1);
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(logicalDevice, commandPool);

    VkBufferCopy copyregion = {
        copyregion.srcOffset = 0,
        copyregion.dstOffset = 0,
        copyregion.size = size
    };

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyregion);

    endSingleTimeCommands(logicalDevice, g_queue, commandBuffer, commandPool);

    vkDestroyCommandPool(logicalDevice, commandPool, NULL);
}
