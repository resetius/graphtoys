#include <stddef.h>
#include <render/char.h>

#include <freetype/freetype.h>

#include <font/font_vs.vert.spv.h>
#include <font/font_fs.frag.spv.h>

#include "render_impl.h"
#include "tools.h"

struct CharImpl {
    struct Char base;
    struct RenderImpl* r;

    // vertices

    // texture
    VkImage tex;
    VkDeviceMemory tex_memory;
    VkImageView imageview;
    VkSampler sampler;
};

static void char_render_(struct Char* ch, int x, int y) {
}

static void char_free_(struct Char* ch) {
    if (ch) {
        struct CharImpl* c = (struct CharImpl*)ch;

        vkDestroySampler(c->r->log_dev, c->sampler, NULL);
        vkDestroyImageView(c->r->log_dev, c->imageview, NULL);

        vkDestroyImage(c->r->log_dev, c->tex, NULL);
        vkFreeMemory(c->r->log_dev, c->tex_memory, NULL);
    }
}

struct Char* rend_vulkan_char_new(struct Render* r1, wchar_t ch, void* bm) {
    struct CharImpl* c = calloc(1, sizeof(*c));
    struct RenderImpl* r = (struct RenderImpl*)r1;
    FT_Face face = (FT_Face)bm;
    FT_Bitmap bitmap = face->glyph->bitmap;
    struct Char base = {
        .render = char_render_,
        .free = char_free_
    };
    c->base = base;
    c->base.ch = ch;
    c->base.w = bitmap.width;
    c->base.h = bitmap.rows;
    c->base.left = face->glyph->bitmap_left;
    c->base.top = face->glyph->bitmap_top;
    c->base.advance = face->glyph->advance.x;
    c->r = r;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkDeviceSize imageSize = bitmap.width*bitmap.rows;

    create_buffer(
        r->phy_dev, r->log_dev,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, &stagingBufferMemory);

    char* data;
    vkMapMemory(r->log_dev, stagingBufferMemory, 0, imageSize, 0, (void*)&data);
    if (bitmap.pitch == bitmap.width) {
        memcpy(data, bitmap.buffer, imageSize);
    } else {
        for (int i = 0; i < bitmap.rows; i++) {
            memcpy(&data[i*bitmap.width], bitmap.buffer+bitmap.pitch*i, bitmap.width);
        }
    }
    vkUnmapMemory(r->log_dev, stagingBufferMemory);

    create_image(
        r->phy_dev, r->log_dev,
        bitmap.width,
        bitmap.rows,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &c->tex, &c->tex_memory);

    transition_image_layout(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        c->tex,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    copy_buffer_to_image(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        stagingBuffer, c->tex, bitmap.width, bitmap.rows);

    transition_image_layout(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        c->tex,
        VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(r->log_dev, stagingBuffer, NULL);
    vkFreeMemory(r->log_dev, stagingBufferMemory, NULL);

    c->imageview = create_image_view(
        r->log_dev,
        c->tex, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(r->phy_dev, &properties);

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    if (vkCreateSampler(r->log_dev, &samplerInfo, NULL, &c->sampler) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create texture sampler\n");
        exit(-1);
    }



    return (struct Char*)c;
}
