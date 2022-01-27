#pragma once

#include "vk.h"

struct PhysicalDevice {
    VkPhysicalDevice dev;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkPhysicalDeviceProperties properties;
};

struct Device {
    VkDevice dev;
    VkExtensionProperties* extensions;
    uint32_t n_extensions;
    const char** enabled_extensions;
    int n_enabled_extensions;
};

void device_new(struct Device* dev, VkPhysicalDevice gpu, const char** enable, int n_enable, const VkDeviceQueueCreateInfo* queue_info, int n_queue_info);
int device_ext_is_enabled(struct Device* dev, const char* name);
void device_free(struct Device* dev);

