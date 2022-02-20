#include <stddef.h>
#include <render/char.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "render_impl.h"
#include "tools.h"

struct CharImpl {
    struct Char base;
    struct RenderImpl* r;

    // texture
    struct Texture tex;
};

static void* char_texture_(struct Char* ch) {
    struct CharImpl* c = (struct CharImpl*)ch;
    return &c->tex;
}

static void char_free_(struct Char* ch) {
    if (ch) {
        struct CharImpl* c = (struct CharImpl*)ch;

        vkDestroyImageView(c->r->log_dev, c->tex.view, NULL);

        vkDestroyImage(c->r->log_dev, c->tex.tex, NULL);
        vkFreeMemory(c->r->log_dev, c->tex.memory, NULL);
        free(c);
    }
}

struct Char* rend_vulkan_char_new(struct Render* r1, wchar_t ch, void* bm) {
    struct CharImpl* c = calloc(1, sizeof(*c));
    struct RenderImpl* r = (struct RenderImpl*)r1;
    FT_Face face = (FT_Face)bm;
    FT_Bitmap bitmap = face->glyph->bitmap;
    struct Char base = {
        .free = char_free_,
        .texture = char_texture_,
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

    if (imageSize == 0) {
        free(c);
        return NULL;
    }

    create_buffer(
        r->memory_properties, r->log_dev,
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
        r->memory_properties, r->log_dev,
        bitmap.width,
        bitmap.rows,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &c->tex.tex, &c->tex.memory);

    transition_image_layout(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        c->tex.tex,
        VK_FORMAT_R8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


    copy_buffer_to_image(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        stagingBuffer, c->tex.tex, bitmap.width, bitmap.rows);

    transition_image_layout(
        r->graphics_family,
        r->g_queue,
        r->log_dev,
        c->tex.tex,
        VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(r->log_dev, stagingBuffer, NULL);
    vkFreeMemory(r->log_dev, stagingBufferMemory, NULL);

    c->tex.view = create_image_view(
        r->log_dev,
        c->tex.tex, VK_FORMAT_R8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    return (struct Char*)c;
}
