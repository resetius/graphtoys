#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "device.h"

void device_new(struct Device* dev, VkPhysicalDevice gpu, const char** enable, int n_enable, const VkDeviceQueueCreateInfo* queue_info, int n_queue_info) {
    memset(dev, 0, sizeof(*dev));
    vkEnumerateDeviceExtensionProperties(gpu, NULL, &dev->n_extensions, NULL);
    dev->extensions = malloc(dev->n_extensions*sizeof(VkExtensionProperties));
    dev->enabled_extensions = malloc(dev->n_extensions*sizeof(const char*));    
    vkEnumerateDeviceExtensionProperties(gpu, NULL, &dev->n_extensions, dev->extensions);

    for (int i = 0; i < dev->n_extensions; i++) {
        for (int j = 0; j < n_enable; j++) {
            if (!strcmp(dev->extensions[i].extensionName, enable[j])) {
                dev->enabled_extensions[dev->n_enabled_extensions++] = dev->extensions[i].extensionName;
                break;
            }
        }
        printf("'%s'\n", dev->extensions[i].extensionName);    
    }
    printf("enabling the following (%d of %d):\n", dev->n_enabled_extensions, n_enable);
    for (int i = 0; i < dev->n_enabled_extensions; i++) {
        printf("'%s'\n", dev->enabled_extensions[i]);
    }

    VkPhysicalDeviceFeatures features = {
        //.samplerAnisotropy = VK_TRUE,
        .vertexPipelineStoresAndAtomics = VK_TRUE,
        .largePoints = VK_TRUE,
    };

    VkDeviceCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queue_info,
        .queueCreateInfoCount = n_queue_info,
        .pEnabledFeatures = &features,
        .enabledExtensionCount = dev->n_enabled_extensions,
        .ppEnabledExtensionNames = dev->enabled_extensions,
        .enabledLayerCount = 0
    };

    if (vkCreateDevice(gpu, &info, NULL, &dev->dev) != VK_SUCCESS) {
        fprintf(stderr, "Cannot create logical device\n");
        exit(-1);
    }    
}

int device_ext_is_enabled(struct Device* dev, const char* name) {
    for (int i = 0; i < dev->n_enabled_extensions; i++) {
        if (!strcmp(dev->enabled_extensions[i], name)) {
            return 1;
        }
    }
    return 0;
}

void device_free(struct Device* dev) {
    free(dev->extensions);
    free(dev->enabled_extensions);
}

