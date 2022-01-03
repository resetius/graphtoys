#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#include <render/pipeline.h>
#include <render/render.h>

#include "render_impl.h"
#include "tools.h"

struct UniformBlock {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkDeviceSize size;
    VkDescriptorSetLayoutBinding layoutBinding;
};

struct PipelineImpl {
    struct Pipeline base;
    struct RenderImpl* r;

    struct UniformBlock* uniforms;
    int n_uniforms;

    VkBuffer* buffers;
    VkDeviceMemory* bufferMemories;
    VkDeviceSize* offsets;
    int n_buffers;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSet* descriptorSets;
    int n_descriptorSets;

    int n_vertices;
};

struct PipelineBuilderImpl {
    struct PipelineBuilder base;
    struct RenderImpl* r;

    struct UniformBlock uniforms[100];
    int n_uniforms;
    struct UniformBlock* cur_uniform;
};

static void buffer_update_(VkDevice dev, VkDeviceMemory memory, const void* data, int offset, int size) {
    void* dest;
    vkMapMemory(dev, memory, offset, size, 0, &dest);
    memcpy(dest, data, size);
    vkUnmapMemory(dev, memory);
}

static void uniform_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    buffer_update_(r->log_dev, p->uniforms[id].uniformBufferMemory, data, offset, size);
}

static void buffer_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    buffer_update_(r->log_dev, p->bufferMemories[id], data, offset, size);
}

static void run(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    VkCommandBuffer buffer = r->buffer;

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->graphicsPipeline);

    vkCmdBindVertexBuffers(
        buffer,
        0,
        p->n_buffers,
        p->buffers,
        p->offsets);

    vkCmdBindDescriptorSets(
        buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        p->pipelineLayout,
        0,
        p->n_descriptorSets,
        p->descriptorSets, 0, NULL);

    vkCmdDraw(
        buffer,
        p->n_vertices, // vertices
        1, // instance count -- just the 1
        0, // vertex offet -- any offsets to add
        0);// first instance -- since no instancing, is set to 0
}


static struct PipelineBuilder* begin_uniform(
    struct PipelineBuilder*p1,
    int binding,
    const char* name,
    int size)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct RenderImpl* r = p->r;

    // TODO: checkptr
    p->cur_uniform = &p->uniforms[p->n_uniforms++];

    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT // TODO
    };

    VkDeviceSize uboBufferSize = size;
    p->cur_uniform->size = size;
    p->cur_uniform->layoutBinding = uboLayoutBinding;

    create_buffer(
        r->phy_dev, r->log_dev,
        uboBufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &p->cur_uniform->uniformBuffer,
        &p->cur_uniform->uniformBufferMemory);

    return p1;
}

static struct PipelineBuilder* end_uniform(struct PipelineBuilder *p1)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_uniform = NULL;
    return p1;
}

static struct PipelineBuilder* begin_program(struct PipelineBuilder* p) {
    return p;
}

static struct PipelineBuilder* end_program(struct PipelineBuilder* p) {
    return p;
}

static struct PipelineBuilder* add_vs(struct PipelineBuilder* p, struct ShaderCode shader) {
}

static struct PipelineBuilder* add_fs(struct PipelineBuilder* p, struct ShaderCode shader) {
}

static struct Pipeline* build(struct PipelineBuilder* p1) {
    struct PipelineImpl* pl = calloc(1, sizeof(*pl));
    struct Pipeline base = {
        .run = run,
        .uniform_update = uniform_update,
        .buffer_update = buffer_update
    };
    pl->base = base;

    return (struct Pipeline*)pl;
}

struct PipelineBuilder* pipeline_builder_vulkan(struct Render* r) {
    struct PipelineBuilderImpl* p = calloc(1, sizeof(*p));

    struct PipelineBuilder base = {
        .begin_uniform = begin_uniform,
        .end_uniform = end_uniform,
        .begin_program = begin_program,
        .end_program = end_program,
        .build = build
    };
    p->base = base;
    p->r = (struct RenderImpl*)r;

    return (struct PipelineBuilder*)p;
}
