#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tools.h"

static uint32_t findMemoryTypeIndex(VkPhysicalDeviceMemoryProperties* props, uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
    for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (props->memoryTypes[i].propertyFlags & flags) == flags)
        {
            return i;
        }
    }

    fprintf(stderr, "Failed to find suitable memory type\n");
    exit(-1);
    return 0;
}

void create_buffer(
    VkPhysicalDeviceMemoryProperties props,
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

    assert(size);

    if (vkCreateBuffer(logicalDevice, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create vertex buffer\n");
        exit(-1);
    }

    VkMemoryRequirements memrequirements;
    vkGetBufferMemoryRequirements(logicalDevice, *buffer, &memrequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memrequirements.size,
        .memoryTypeIndex = findMemoryTypeIndex(&props, memrequirements.memoryTypeBits, properties)
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


void create_image(
    VkPhysicalDeviceMemoryProperties props,
    VkDevice device,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage* image,
    VkDeviceMemory* imageMemory)
{
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if (vkCreateImage(device, &imageInfo, NULL, image) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create image\n");
        exit(-1);
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, *image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryTypeIndex(
            &props, memRequirements.memoryTypeBits, properties)
    };

    if (vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
        fprintf(stderr, "Failed to allocate image memory\n");
        exit(-1);
    }

    vkBindImageMemory(device, *image, *imageMemory, 0);
}

VkImageView create_image_view(
    VkDevice logDev, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,

        .subresourceRange.aspectMask = aspectFlags,
	    .subresourceRange.baseMipLevel = 0,
	    .subresourceRange.levelCount = 1,

        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1
    };

    VkImageView imageView;
    if (vkCreateImageView(logDev, &viewInfo, NULL, &imageView) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create texture image view\n");
        exit(-1);
    }

    return imageView;
}


void transition_image_layout(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
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

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        fprintf(stderr, "Unsupported layout transition\n");
        exit(-1);
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, NULL,
        0, NULL,
        1, &barrier
        );

    endSingleTimeCommands(logicalDevice, g_queue, commandBuffer, commandPool);

    vkDestroyCommandPool(logicalDevice, commandPool, NULL);
}


void copy_buffer_to_image(
    int graphics_family,
    VkQueue g_queue,
    VkDevice logicalDevice,
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
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

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            width,
            height,
            1
        }
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    endSingleTimeCommands(logicalDevice, g_queue, commandBuffer, commandPool);

     vkDestroyCommandPool(logicalDevice, commandPool, NULL);
}
