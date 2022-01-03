#include <stdio.h>
#include <stdlib.h>

#include "rendertarget.h"
#include "render_impl.h"

static VkImageView image_view_create(
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

void rt_init(struct RenderTarget* rt, struct RenderImpl* r) {
    int i;
    rt->n_imageviews = r->sc.n_images;
    rt->imageviews = malloc(rt->n_imageviews*sizeof(VkImageView));

    for (i = 0; i < rt->n_imageviews; i++) {
        rt->imageviews[i] = image_view_create(
            r->log_dev,
            r->sc.images[i],
            r->sc.im_format,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    rt->n_framebuffers = rt->n_imageviews;
    rt->framebuffers = malloc(rt->n_framebuffers*sizeof(VkFramebuffer));

    for (i = 0; i < rt->n_framebuffers; i++) {
        VkImageView attachments[] = {
            rt->imageviews[i]
        };

        VkFramebufferCreateInfo fbInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = r->rp.rp,
            .attachmentCount = (uint32_t)1,
            .pAttachments = attachments,
            .width = r->sc.extent.width,
            .height = r->sc.extent.height,
            .layers = 1
        };

        if (vkCreateFramebuffer(r->log_dev, &fbInfo, NULL, &rt->framebuffers[i]) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create framebuffers\n");
            exit(-1);
		}
    }

    rt->dev = r->log_dev;
}

void rt_destroy(struct RenderTarget* rt) {
    int i;
    for (i = 0; i < rt->n_imageviews; i++) {
        vkDestroyImageView(rt->dev, rt->imageviews[i], NULL);
    }
    for (i = 0; i < rt->n_framebuffers; i++) {
        vkDestroyFramebuffer(rt->dev, rt->framebuffers[i], NULL);
    }
    free(rt->imageviews);
    free(rt->framebuffers);
}