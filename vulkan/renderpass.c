#include <stdio.h>
#include <stdlib.h>

#include "renderpass.h"

void rp_init(struct RenderPass* r, VkDevice logDev, VkFormat swapChainImageFormat) {
	VkAttachmentDescription colorAttachment = {
        .format = swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,

        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

	VkAttachmentReference colorAttachRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

	VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachRef
    };

	VkAttachmentDescription attachments[] = { colorAttachment };
    uint32_t n_attachments = 1;

	VkRenderPassCreateInfo rpCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = n_attachments,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

	if (vkCreateRenderPass(logDev, &rpCreateInfo, NULL, &r->rp) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create renderpass\n");
        exit(-1);
    }
    r->dev = logDev;
}

void rp_begin(
    struct RenderPass* r, VkClearValue value, VkCommandBuffer commands,
    VkFramebuffer framebuffer, VkExtent2D extent)
{
    VkRenderPassBeginInfo rpBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = r->rp,
        .framebuffer = framebuffer,
        .renderArea.offset = { 0,0 },
        .renderArea.extent = extent,

        .pClearValues = &value,
        .clearValueCount = 1
    };

    vkCmdBeginRenderPass(commands, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void rp_end(struct RenderPass* r, VkCommandBuffer commands) {
    vkCmdEndRenderPass(commands);
}

void rp_destroy(struct RenderPass* r) {
    vkDestroyRenderPass(r->dev, r->rp, NULL);
}
