#include <stdio.h>
#include <stdlib.h>

#include "swapchain.h"
#include "render_impl.h"
#include "tools.h"

// TODO
static uint32_t max(uint32_t a, uint32_t b) {
    return a>b?a:b;
}

static uint32_t min(uint32_t a, uint32_t b) {
    return a<b?a:b;
}

static VkSurfaceFormatKHR choose_format(VkSurfaceFormatKHR* formats, int n) {
    int i;
    if (n == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        VkSurfaceFormatKHR format = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
        return format;
	}

    for (i = 0; i < n; i++) {
        VkSurfaceFormatKHR* format = &formats[i];
        if (format->format == VK_FORMAT_B8G8R8A8_UNORM
            && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return *format;
    	}
	}

	return formats[0];
}

static VkPresentModeKHR choose_mode(VkPresentModeKHR* modes, int n_modes) {

	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
    int i;

    for (i = 0; i < n_modes; i++) {
        VkPresentModeKHR* mode = &modes[i];
        if (*mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return *mode;
        } else if (*mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = *mode;
		}
	}

    return bestMode;
}

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR* caps) {

    if (caps->currentExtent.width != (uint32_t)-1) {
        return caps->currentExtent;
    } else {
		VkExtent2D extent = { 800, 600 };

		extent.width = max(
            caps->minImageExtent.width,
            min(caps->maxImageExtent.width, extent.width));
		extent.height = max(
            caps->minImageExtent.height,
            min(caps->maxImageExtent.height, extent.height));

		return extent;
	}
}

void sc_init(struct SwapChain* sc, struct RenderImpl* r) {
    sc->depth_format = VK_FORMAT_D24_UNORM_S8_UINT; // TODO: detect format
    //sc->depth_format = VK_FORMAT_D32_SFLOAT;
    VkSurfaceFormatKHR surfaceFormat = choose_format(r->formats, r->n_formats);

    VkPresentModeKHR presentMode = choose_mode(r->modes, r->n_modes);

    VkExtent2D extent = choose_extent(&r->caps);

    uint32_t imageCount = r->caps.minImageCount;

    if (r->caps.maxImageCount > 0 && imageCount > r->caps.maxImageCount) {
        imageCount = r->caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = r->surface,

        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = r->caps.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    uint32_t queueFamilyIndices[] = {
        (uint32_t)r->graphics_family,
        (uint32_t)r->present_family
    };

    if (r->graphics_family != r->present_family) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = NULL;
    }

    if (vkCreateSwapchainKHR(r->log_dev, &createInfo, NULL, &sc->swapchain) != VK_SUCCESS)
    {
        fprintf(stderr, "failed to create swap chain !\n");
        exit(-1);
    }

    vkGetSwapchainImagesKHR(r->log_dev, sc->swapchain, &imageCount, NULL);

    sc->images = malloc(imageCount*sizeof(VkImage));
    sc->n_images = imageCount;

    vkGetSwapchainImagesKHR(r->log_dev, sc->swapchain, &imageCount, sc->images);

    sc->im_format = surfaceFormat.format;
    sc->extent = extent;
    sc->r = r;


    create_image(
        r->phy_dev,
        r->log_dev,
        sc->extent.width,
        sc->extent.height,
        sc->depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &sc->depth_image,
        &sc->depth_image_memory);
}

void sc_destroy(struct SwapChain* sc) {
    free(sc->images);
    vkDestroyImage(sc->r->log_dev, sc->depth_image, NULL);
    vkFreeMemory(sc->r->log_dev, sc->depth_image_memory, NULL);
    vkDestroySwapchainKHR(sc->r->log_dev, sc->swapchain, NULL);
}
