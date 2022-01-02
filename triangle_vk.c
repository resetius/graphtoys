#include <stdlib.h>
#include <stdio.h>

#include <vulkan/render_impl.h>
#include <vulkan/tools.h>

#include "object.h"
#include "linmath.h"

#include "triangle_vertex_shader.vert.spv.h"
#include "triangle_fragment_shader.frag.spv.h"

struct Vertex
{
    vec2 pos;
    vec3 col;
};

static const struct Vertex vertices[3] =
{
    { { -0.6f, -0.4f }, { 1.f, 0.f, 0.f } },
    { {  0.6f, -0.4f }, { 0.f, 1.f, 0.f } },
    { {   0.f,  0.6f }, { 0.f, 0.f, 1.f } }
};

struct Triangle {
    struct Object base;
    struct RenderImpl* r;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
};

static void trvk_draw(struct Object* obj, struct DrawContext* ctx) {
}

static void trvk_free(struct Object* obj) {
}

struct Object* trvk_new(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    struct Triangle* tr = calloc(1, sizeof(*tr));
    struct Object base = {
        .free = trvk_free,
        .draw = trvk_draw
    };

    tr->base = base;
    tr->r = r;

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

    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutBinding layoutBindings[] = {uboLayoutBinding};
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = layoutBindings
    };

    if (vkCreateDescriptorSetLayout(r->log_dev, &layoutCreateInfo, NULL, &descriptorSetLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        exit(-1);
	}

    VkVertexInputAttributeDescription attributeDescriptions[2]; // pos, col
    VkVertexInputAttributeDescription posDescr = {
        .binding = 0,
        .location = 2, // vCol
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(struct Vertex, pos)
    };
    attributeDescriptions[0] = posDescr;
    VkVertexInputAttributeDescription colDescr = {
        .binding = 0,
        .location = 1, // vPos
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(struct Vertex, col)
    };
    attributeDescriptions[1] = colDescr;

    VkDeviceSize bufferSize = sizeof(vertices);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // create staging
    create_buffer(
        r->phy_dev, r->log_dev,
        bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);
    // end of create staging

    void* data;
    vkMapMemory(
        r->log_dev,
        stagingBufferMemory,
        0, // offet
        bufferSize,// size
        0,// flag
        &data); // copy buffer memory to data
    memcpy(data, vertices, bufferSize);
    vkUnmapMemory(
        r->log_dev,
        stagingBufferMemory);
    // createbuffer
    create_buffer(
        r->phy_dev, r->log_dev,
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &tr->vertexBuffer,
        &tr->vertexBufferMemory);
    // copybuffer
    copy_buffer(r->graphics_family, r->g_queue, r->log_dev, stagingBuffer, tr->vertexBuffer, bufferSize);

    vkDestroyBuffer(r->log_dev, stagingBuffer, NULL);
	vkFreeMemory(r->log_dev, stagingBufferMemory, NULL);
    printf("Vertex buffer created\n");

    for (int i = 0; i < r->sc.n_images; i++) {

    }

    return (struct Object*)tr;
}
