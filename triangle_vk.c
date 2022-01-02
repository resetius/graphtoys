#include <stdlib.h>
#include <stdio.h>

#include <vulkan/render_impl.h>

#include "object.h"

#include "triangle_vertex_shader.vert.spv.h"
#include "triangle_fragment_shader.frag.spv.h"

struct Triangle {
    struct Object base;
    struct RenderImpl* r;
};

static void trvk_draw(struct Object* obj, struct DrawContext* ctx) {
}

static void trvk_free(struct Object* obj) {
}

struct Object* trvk_new(struct Render* r1) {
    struct Triangle* tr = calloc(1, sizeof(*tr));
    struct Object base = {
        .free = trvk_free,
        .draw = trvk_draw
    };

    tr->base = base;
    tr->r = (struct RenderImpl*)r1;

    VkShaderModuleCreateInfo cInfo1 = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = triangle_vertex_shader_size,
        .pCode = (const uint32_t*)(triangle_vertex_shader)
    };

    VkShaderModule vertShader;
	if (vkCreateShaderModule(tr->r->log_dev, &cInfo1, NULL, &vertShader) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module\n");
	}
    printf("Vertex shader loaded\n");


    VkShaderModuleCreateInfo cInfo2 = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = triangle_fragment_shader_size,
        .pCode = (const uint32_t*)(triangle_fragment_shader)
    };

    VkShaderModule fragShader;
	if (vkCreateShaderModule(tr->r->log_dev, &cInfo2, NULL, &fragShader) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create shader module\n");
	}
    printf("Fragment shader loaded\n");

    return (struct Object*)tr;
}
