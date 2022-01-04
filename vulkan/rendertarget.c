#include <stdio.h>
#include <stdlib.h>

#include "rendertarget.h"
#include "render_impl.h"
#include "tools.h"

void rt_init(struct RenderTarget* rt, struct RenderImpl* r) {
    int i;
    rt->n_imageviews = r->sc.n_images;
    rt->imageviews = malloc(rt->n_imageviews*sizeof(VkImageView));

    for (i = 0; i < rt->n_imageviews; i++) {
        rt->imageviews[i] = create_image_view(
            r->log_dev,
            r->sc.images[i],
            r->sc.im_format,
            VK_IMAGE_ASPECT_COLOR_BIT);
    }

    rt->depth_imageview = create_image_view(r->log_dev, r->sc.depth_image,
                      r->sc.depth_format,
                      VK_IMAGE_ASPECT_DEPTH_BIT);

    rt->n_framebuffers = rt->n_imageviews;
    rt->framebuffers = malloc(rt->n_framebuffers*sizeof(VkFramebuffer));

    for (i = 0; i < rt->n_framebuffers; i++) {
        VkImageView attachments[] = {
            rt->imageviews[i],
            rt->depth_imageview
        };
        int n_attachments = 2;

        VkFramebufferCreateInfo fbInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = r->rp.rp,
            .attachmentCount = n_attachments,
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
    vkDestroyImageView(rt->dev, rt->depth_imageview, NULL);
    for (i = 0; i < rt->n_framebuffers; i++) {
        vkDestroyFramebuffer(rt->dev, rt->framebuffers[i], NULL);
    }
    free(rt->imageviews);
    free(rt->framebuffers);
}
