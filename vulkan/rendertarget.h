#pragma once

#include <vulkan/vulkan.h>

struct RenderImpl;
struct RenderTarget {
    VkImageView* imageviews;
    int n_imageviews;
	VkFramebuffer* framebuffers;
    int n_framebuffers;
    VkDevice dev;
};

void rt_init(struct RenderTarget* rt, struct RenderImpl* r);
void rt_destroy(struct RenderTarget* rt);
