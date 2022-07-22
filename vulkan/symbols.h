#ifndef L0_FUNC
#define L0_FUNC(name)
#endif


L0_FUNC(vkCreateInstance);
L0_FUNC(vkEnumerateInstanceExtensionProperties);

#undef L0_FUNC

#ifndef L1_FUNC
#define L1_FUNC(name)
#endif

L1_FUNC(vkEnumeratePhysicalDevices);
L1_FUNC(vkGetPhysicalDeviceProperties);
L1_FUNC(vkGetPhysicalDeviceFormatProperties);
L1_FUNC(vkGetPhysicalDeviceMemoryProperties);
L1_FUNC(vkGetPhysicalDeviceFeatures);
L1_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
L1_FUNC(vkCreateDevice);
L1_FUNC(vkGetDeviceProcAddr);
L1_FUNC(vkDestroyInstance);
L1_FUNC(vkEnumerateDeviceExtensionProperties);
L1_FUNC(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR);
L1_FUNC(vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR);
L1_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
L1_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
L1_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
L1_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
L1_FUNC(vkGetPhysicalDeviceImageFormatProperties);
L1_FUNC(vkDestroySurfaceKHR);

#undef L1_FUNC

#ifndef L2_FUNC
#define L2_FUNC(name)
#endif

L2_FUNC(vkAcquireNextImageKHR);
L2_FUNC(vkAllocateDescriptorSets);
L2_FUNC(vkCmdBlitImage);
L2_FUNC(vkMapMemory);
L2_FUNC(vkUnmapMemory);
L2_FUNC(vkGetDeviceQueue);
L2_FUNC(vkDeviceWaitIdle);
L2_FUNC(vkDestroyDevice);
L2_FUNC(vkCreateSemaphore);
L2_FUNC(vkCreateCommandPool);
L2_FUNC(vkResetCommandPool);
L2_FUNC(vkAllocateCommandBuffers);
L2_FUNC(vkBeginCommandBuffer);
L2_FUNC(vkResetCommandBuffer);
L2_FUNC(vkCmdPipelineBarrier);
L2_FUNC(vkCmdClearColorImage);
L2_FUNC(vkEndCommandBuffer);
L2_FUNC(vkQueueSubmit);
L2_FUNC(vkFreeCommandBuffers);
L2_FUNC(vkDestroyCommandPool);
L2_FUNC(vkDestroySemaphore);
L2_FUNC(vkAllocateMemory);
L2_FUNC(vkBindBufferMemory);
L2_FUNC(vkBindImageMemory);
L2_FUNC(vkCmdBindIndexBuffer);
L2_FUNC(vkCmdBeginRenderPass);
L2_FUNC(vkCmdBindDescriptorSets);
L2_FUNC(vkCmdBindPipeline);
L2_FUNC(vkCmdBindVertexBuffers);
L2_FUNC(vkCmdCopyBuffer);
L2_FUNC(vkCmdCopyBufferToImage);
L2_FUNC(vkCmdDispatch);
L2_FUNC(vkCmdDraw);
L2_FUNC(vkCmdDrawIndexed);
L2_FUNC(vkCmdEndRenderPass);
L2_FUNC(vkCmdSetScissor);
L2_FUNC(vkCmdSetViewport);
L2_FUNC(vkCreateBuffer);
L2_FUNC(vkCreateComputePipelines);
L2_FUNC(vkCreateDescriptorPool);
L2_FUNC(vkCreateDescriptorSetLayout);
L2_FUNC(vkCreateFence);
L2_FUNC(vkCreateFramebuffer);
L2_FUNC(vkCreateGraphicsPipelines);
L2_FUNC(vkCreateImage);
L2_FUNC(vkCreateImageView);
L2_FUNC(vkCreatePipelineLayout);
L2_FUNC(vkCreateRenderPass);
L2_FUNC(vkCreateSampler);
L2_FUNC(vkCreateShaderModule);
L2_FUNC(vkCreateSwapchainKHR);
L2_FUNC(vkCreateQueryPool);
L2_FUNC(vkDestroyBuffer);
L2_FUNC(vkDestroyDescriptorPool);
L2_FUNC(vkDestroyDescriptorSetLayout);
L2_FUNC(vkDestroyFence);
L2_FUNC(vkDestroyFramebuffer);
L2_FUNC(vkDestroyImage);
L2_FUNC(vkDestroyImageView);
L2_FUNC(vkDestroyPipeline);
L2_FUNC(vkDestroyPipelineLayout);
L2_FUNC(vkDestroyRenderPass);
L2_FUNC(vkDestroySampler);
L2_FUNC(vkDestroyShaderModule);
L2_FUNC(vkDestroySwapchainKHR);
L2_FUNC(vkDestroyQueryPool);
L2_FUNC(vkFreeMemory);
L2_FUNC(vkGetBufferMemoryRequirements);
L2_FUNC(vkGetImageMemoryRequirements);
L2_FUNC(vkGetImageSubresourceLayout);
L2_FUNC(vkGetSwapchainImagesKHR);
L2_FUNC(vkGetQueryPoolResults);
L2_FUNC(vkQueuePresentKHR);
L2_FUNC(vkQueueWaitIdle);
L2_FUNC(vkResetFences);
L2_FUNC(vkCmdUpdateBuffer);
L2_FUNC(vkCmdWriteTimestamp);
L2_FUNC(vkCmdResetQueryPool);
L2_FUNC(vkUpdateDescriptorSets);
L2_FUNC(vkWaitForFences);

#undef L2_FUNC
