#include <string.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

#include <render/pipeline.h>
#include <render/render.h>

#include "render_impl.h"

struct UniformBlock {
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
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
