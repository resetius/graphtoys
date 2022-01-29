#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <render/pipeline.h>
#include <render/render.h>

#include "render_impl.h"
#include "tools.h"
#include "buffer.h"

struct BufferDescriptor {
    int stride;
    VkVertexInputBindingDescription descr;
    VkVertexInputAttributeDescription attr[20];
    int n_attr;
};

struct Buffer {
    struct BufferImpl base;
    int stride;
    int n_vertices;
    int binding;
};

struct UniformBlock {
    struct BufferImpl base;
    VkDescriptorSetLayoutBinding layoutBinding;
};

struct PipelineImpl {
    struct Pipeline base;
    struct RenderImpl* r;

    struct UniformBlock* uniforms;
    int n_uniforms;

    struct BufferDescriptor* buf_descr;
    int n_buf_descr;

    struct Buffer* buffers;
    int n_buffers;
    int buf_cap;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkPipeline computePipeline;

    VkDescriptorSet* descriptorSets;
    int* descriptorSetFlags;
    VkDescriptorSet* currentDescriptorSet;
    int n_descriptor_sets;

    VkSampler* samplers;
    int n_samplers;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout layout;
    VkDescriptorSetLayout texLayout;

    struct BufferManager* b; // TODO: remove me
    int owns; // TODO: remove me
};

struct Sampler {
    struct VkSamplerCreateInfo info;
    VkDescriptorSetLayoutBinding layoutBinding;
    int binding;
};

struct PipelineBuilderImpl {
    struct PipelineBuilder base;
    struct RenderImpl* r;

    struct UniformBlock uniforms[100];
    int n_uniforms;
    struct UniformBlock* cur_uniform;

    VkShaderModule vert_shaders[100];
    int n_vert_shaders;

    VkShaderModule frag_shaders[100];
    int n_frag_shaders;

    VkShaderModule comp_shaders[100];
    int n_comp_shaders;

    struct BufferDescriptor buffers[100];
    struct BufferDescriptor* cur_buffer;
    int n_buffers;

    struct Buffer allocated[100];
    int n_allocated;

    struct Sampler samplers[100];
    int n_samplers;

    VkPipelineShaderStageCreateInfo* shader_stages;
    VkVertexInputBindingDescription* binding_descrs;
    VkVertexInputAttributeDescription* attr_descrs;
    int n_attrs;
    VkDescriptorSetLayoutBinding* layout_bindings;
    VkDescriptorSetLayoutBinding* texture_layout_bindings;

    int enable_depth;
    int enable_blend;

    enum GeometryType geometry;
    VkCullModeFlagBits cull_mode;

    struct BufferManager* b; // TODO: remove me
    int owns; // TODO: remove me
};

static uint32_t primitive_topology(enum GeometryType geometry) {
    uint32_t ret = -1;
    switch (geometry) {
    case GEOM_TRIANGLES:
        ret = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case GEOM_POINTS:
        ret = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    default:
        assert(0);
        break;
    }
    return ret;
}

static int buffer_assign(struct Pipeline* p1, int id, int buffer_id) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(id < p->n_buf_descr);
    struct BufferDescriptor* descr = &p->buf_descr[id];

    if (p->n_buffers >= p->buf_cap) {
        p->buf_cap = (p->buf_cap+1)*2;
        p->buffers = realloc(p->buffers, p->buf_cap*sizeof(struct Buffer));
    }
    int logical_id = p->n_buffers++; // logical id
    struct Buffer* buf = &p->buffers[logical_id];
    memset(buf, 0, sizeof(*buf));
    memcpy(&buf->base, p->b->get(p->b, buffer_id), sizeof(buf->base));

    buf->stride = descr->stride;
    buf->n_vertices = buf->base.size / descr->stride;
    buf->binding = id; // ?

    return logical_id;
}

static void uniform_assign(struct Pipeline* p1, int uniform_id, int buffer_id)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;
    assert(uniform_id < p->n_uniforms);
    for (i = 0; i < p->r->sc.n_images; i++) {
        p->descriptorSetFlags[i] = 0; // rebuild descriptor set
    }
    memcpy(&p->uniforms[uniform_id].base,
           p->b->get(p->b, buffer_id),
           sizeof(p->uniforms[uniform_id].base));
}

static void uniform_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    p->b->update(p->b, p->uniforms[id].base.base.id, data, offset, size);
}

static void buffer_update_(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(id < p->n_buffers);

    struct Buffer* buf = &p->buffers[id];
    p->b->update(p->b, buf->base.base.id, data, offset, size);
}

static int buffer_create_(struct Pipeline* p1, enum BufferType type, enum BufferMemoryType mem_type, int binding, const void* data, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(binding < p->n_buf_descr);
    struct BufferDescriptor* descr = &p->buf_descr[binding];
    if (p->n_buffers >= p->buf_cap) {
        p->buf_cap = (p->buf_cap+1)*2;
        p->buffers = realloc(p->buffers, p->buf_cap*sizeof(struct Buffer));
    }
    int buffer_id = p->n_buffers++;
    struct Buffer* buf = &p->buffers[buffer_id];
    buf->stride = descr->stride;
    buf->n_vertices = size / descr->stride;
    buf->binding = binding;

    int id = p->b->create(p->b, type, mem_type, data, size);
    memcpy(&buf->base, p->b->get(p->b, id), sizeof(buf->base));

    return buffer_id;
}

static void use_texture(struct Pipeline* p1, void* texture) {
    struct PipelineImpl* pl = (struct PipelineImpl*)p1;
    struct RenderImpl* r = pl->r;
    struct Texture* tex = (struct Texture*)texture;

    pl->currentDescriptorSet = &tex->set;

    if (tex->has_set) {
        return;
    }

    tex->has_set = 1;

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pl->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pl->texLayout,
    };

    if (vkAllocateDescriptorSets(r->log_dev, &allocInfo, &tex->set) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate texture descriptor set\n");
        exit(-1);
    }

    VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = tex->view,
        .sampler = pl->samplers[0] // TODO
    };

    VkWriteDescriptorSet descriptorWrites = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = tex->set,
        .dstBinding = 0, // TODO
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(r->log_dev, 1, &descriptorWrites, 0, NULL);
}

static void start(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    VkCommandBuffer buffer = r->buffer;

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, p->graphicsPipeline);
}

static void storage_swap(struct Pipeline* p1, int dst, int src) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;
    // TODO performance
    for (i = 0; i < p->r->sc.n_images; i++) {
        p->descriptorSetFlags[i] = 0; // rebuild descriptor set
    }
    int t = p->uniforms[dst].layoutBinding.binding;
    p->uniforms[dst].layoutBinding.binding = p->uniforms[src].layoutBinding.binding;
    p->uniforms[src].layoutBinding.binding = t;
}

static void start_compute(struct Pipeline* p1, int sx, int sy, int sz) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    VkCommandBuffer buffer = r->buffer; // use different command buffer?

    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, p->computePipeline);

}

static VkDescriptorSet currentDescriptorSet(struct PipelineImpl* p)
{
    struct RenderImpl* r = p->r;
    int i = r->image_index;

    if (!p->descriptorSetFlags[i]) {
        for (int j = 0; j < p->n_uniforms; j++) {
            int idx = p->uniforms[j].base.n_buffers > 1
                ? i
                : 0;

            VkDescriptorBufferInfo info = {
                .buffer = p->uniforms[j].base.buffer[idx],
                .offset = 0,
                .range = p->uniforms[j].base.size
            };

            VkWriteDescriptorSet uboDescWrites = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = NULL,
                .dstSet = p->descriptorSets[i],
                .dstBinding = p->uniforms[j].layoutBinding.binding,
                .dstArrayElement = 0,
                .descriptorType = p->uniforms[j].layoutBinding.descriptorType,
                .descriptorCount = 1,
                .pBufferInfo = &info,
                .pImageInfo = NULL,
                .pTexelBufferView = NULL,
            };

            VkWriteDescriptorSet descWrites[] = {uboDescWrites};
            vkUpdateDescriptorSets(
                r->log_dev,
                1,
                descWrites,
                0,
                NULL);
        }

        p->descriptorSetFlags[i] = 1;
    }

    return p->descriptorSets[i];
}

static void draw(struct Pipeline* p1, int id) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;

    VkCommandBuffer buffer = r->buffer;

    if (id < p->n_buffers) {
        VkDeviceSize offset = 0;
        struct Buffer* buf = &p->buffers[id];

        //printf("Draw %d %d\n", id, buf->n_vertices);

        VkDescriptorSet curSet = currentDescriptorSet(p);
        VkDescriptorSet textureDescriptorSet = NULL;
        int n_sets = 1;
        if (p->currentDescriptorSet) {
            textureDescriptorSet = *p->currentDescriptorSet;
            n_sets = 2;
        }
        VkDescriptorSet use [] = { curSet, textureDescriptorSet };

        vkCmdBindDescriptorSets(
            buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            p->pipelineLayout,
            0,
            n_sets,
            use, 0, NULL);


        vkCmdBindVertexBuffers(
            buffer,
            buf->binding,
            1,
            &buf->base.buffer[0],
            &offset);

        //printf("Use %p\n", p->currentDescriptorSet);

        //printf("vertices: %d\n", buf->n_vertices);
        vkCmdDraw(
            buffer,
            buf->n_vertices, // vertices
            1, // instance count -- just the 1
            0, // vertex offet -- any offsets to add
            0);// first instance -- since no instancing, is set to 0
    }
}

static struct PipelineBuilder* begin_sampler(struct PipelineBuilder*p1, int binding)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct RenderImpl* r = p->r;

    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = r->properties.limits.maxSamplerAnisotropy,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
        .binding = 0, // TODO
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    struct Sampler* sampler = &p->samplers[p->n_samplers++];
    sampler->info = samplerInfo;
    sampler->layoutBinding = samplerLayoutBinding;
    sampler->binding = binding;
    return p1;
}

static struct PipelineBuilder* end_sampler(struct PipelineBuilder*p1)
{
    return p1;
}

static struct PipelineBuilder* uniform_or_storage_add(
    struct PipelineBuilder*p1,
    int binding,
    uint32_t type)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;

    assert(p->cur_uniform == NULL);
    p->cur_uniform = &p->uniforms[p->n_uniforms++];

    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = binding,
        .descriptorCount = 1,
        .descriptorType = type,
        .pImmutableSamplers = NULL,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT|VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT // TODO
    };

    p->cur_uniform->layoutBinding = uboLayoutBinding;
    p->cur_uniform = NULL;

    return p1;
}

static struct PipelineBuilder* storage_add(
    struct PipelineBuilder*p1,
    int binding,
    const char* name)
{
    return uniform_or_storage_add(p1, binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

static struct PipelineBuilder* uniform_add(
    struct PipelineBuilder*p1,
    int binding,
    const char* name)
{
    return uniform_or_storage_add(p1, binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

static struct PipelineBuilder* begin_program(struct PipelineBuilder* p) {
    return p;
}

static struct PipelineBuilder* end_program(struct PipelineBuilder* p) {
    return p;
}


static VkShaderModule add_shader(VkDevice device, struct ShaderCode shader) {
    VkShaderModuleCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shader.size,
        .pCode = (const uint32_t*)(shader.spir_v)
    };

    VkShaderModule result;
    if (vkCreateShaderModule(device, &info, NULL, &result) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create shader module\n");
        exit(-1);
    }
    return result;
}

static struct PipelineBuilder* add_vs(struct PipelineBuilder* p1,struct ShaderCode shader)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->vert_shaders[p->n_vert_shaders++] = add_shader(p->r->log_dev, shader);
    return p1;
}

static struct PipelineBuilder* add_fs(struct PipelineBuilder* p1,struct ShaderCode shader)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->frag_shaders[p->n_frag_shaders++] = add_shader(p->r->log_dev, shader);
    return p1;
}

static struct PipelineBuilder* add_cs(struct PipelineBuilder* p1,struct ShaderCode shader)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->comp_shaders[p->n_comp_shaders++] = add_shader(p->r->log_dev, shader);
    return p1;
}

static struct PipelineBuilder* begin_buffer(struct PipelineBuilder* p1, int stride)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    VkVertexInputBindingDescription descr = {
        .binding = p->n_buffers,
        .stride = stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    // TODO: check ptr
    p->cur_buffer = &p->buffers[p->n_buffers++];
    p->cur_buffer->stride = stride;
    p->cur_buffer->descr = descr;

    return p1;
}

static int get_format(int channels, enum DataType data_type) {
    int fmt = 0;
    switch (data_type) {
    case DATA_FLOAT:
        if (channels == 1) {
            fmt = VK_FORMAT_R32_SFLOAT;
        } else if (channels == 2) {
            fmt = VK_FORMAT_R32G32_SFLOAT;
        } else if (channels == 3) {
            fmt = VK_FORMAT_R32G32B32_SFLOAT;
        } else if (channels == 4) {
            fmt = VK_FORMAT_R32G32B32A32_SFLOAT;
        } else {
            fprintf(stderr, "Wrong number of channels\n");
            exit(-1);
        }
        break;
    case DATA_INT:
        if (channels == 1) {
            fmt = VK_FORMAT_R32_SINT;
        } else if (channels == 2) {
            fmt = VK_FORMAT_R32G32_SINT;
        } else if (channels == 3) {
            fmt = VK_FORMAT_R32G32B32_SINT;
        } else if (channels == 4) {
            fmt = VK_FORMAT_R32G32B32A32_SINT;
        } else {
            fprintf(stderr, "Wrong number of channels\n");
            exit(-1);
        }
        break;
    default:
        fprintf(stderr, "Wrong data type %d\n", data_type);
        exit(-1);
        break;
    }
    return fmt;
}

struct PipelineBuilder* buffer_attribute(
    struct PipelineBuilder* p1,
    int location,
    int channels,
    enum DataType data_type,
    uint64_t offset)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    VkVertexInputAttributeDescription attr = {
        .binding = p->cur_buffer->descr.binding,
        .location = location,
        .format = get_format(channels, data_type),
        .offset = offset
    };
    p->cur_buffer->attr[p->cur_buffer->n_attr++] = attr;
    return p1;
}

struct PipelineBuilder* end_buffer(struct PipelineBuilder* p1)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->n_attrs += p->cur_buffer->n_attr;
    p->cur_buffer = 0;
    return p1;
}

static VkPipelineShaderStageCreateInfo* get_shader_stages(struct PipelineBuilderImpl* p)
{
    VkPipelineShaderStageCreateInfo* infos = malloc(
        (p->n_comp_shaders+p->n_vert_shaders+p->n_frag_shaders)*
        sizeof(VkPipelineShaderStageCreateInfo));
    int i, k;
    k = 0;
    for (i = 0; i < p->n_comp_shaders; i++) {
        VkPipelineShaderStageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = p->comp_shaders[i],
            .pName = "main"
        };
        infos[k++] = info;
    }

    for (i = 0; i < p->n_vert_shaders; i++) {
        VkPipelineShaderStageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = p->vert_shaders[i],
            .pName = "main"
        };
        infos[k++] = info;
    }

    for (i = 0; i < p->n_frag_shaders; i++) {
        VkPipelineShaderStageCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = p->frag_shaders[i],
            .pName = "main"
        };
        infos[k++] = info;
    }
    return (p->shader_stages = infos);
}

static VkVertexInputBindingDescription* get_binding_descrs(struct PipelineBuilderImpl* p)
{
    VkVertexInputBindingDescription* descrs = malloc(p->n_buffers*sizeof(VkVertexInputBindingDescription));
    int i;
    for (i = 0; i < p->n_buffers; i++) {
        descrs[i] = p->buffers[i].descr;
    }
    return (p->binding_descrs = descrs);
}

static VkVertexInputAttributeDescription* get_attr_descrs(struct PipelineBuilderImpl* p)
{
    VkVertexInputAttributeDescription* descrs = malloc(p->n_attrs*sizeof(VkVertexInputAttributeDescription));
    int i,j,k;
    k = 0;
    for (i = 0; i < p->n_buffers; i++) {
        for (j = 0; j < p->buffers[i].n_attr; j++) {
            descrs[k++] = p->buffers[i].attr[j];
        }
    }
    return (p->attr_descrs = descrs);
}

struct VkDescriptorSetLayoutBinding* get_layout_bindings(struct PipelineBuilderImpl*p)
{
    VkDescriptorSetLayoutBinding* infos = malloc(p->n_uniforms*sizeof(VkDescriptorSetLayoutBinding));
    int k = 0, i;
    for (i = 0; i < p->n_uniforms; i++) {
        infos[k++] = p->uniforms[i].layoutBinding;
    }
    return (p->layout_bindings = infos);
}

struct VkDescriptorSetLayoutBinding* get_texture_layout_bindings(struct PipelineBuilderImpl*p)
{
    VkDescriptorSetLayoutBinding* infos = malloc(p->n_samplers*sizeof(VkDescriptorSetLayoutBinding));
    int k = 0, i;
    for (i = 0; i < p->n_samplers; i++) {
        infos[k++] = p->samplers[i].layoutBinding;
    }
    return (p->texture_layout_bindings = infos);
}

static void builder_free(struct PipelineBuilderImpl* p) {
    int i;
    for (i = 0; i < p->n_vert_shaders; i++) {
        vkDestroyShaderModule(p->r->log_dev, p->vert_shaders[i], NULL);
    }
    for (i = 0; i < p->n_frag_shaders; i++) {
        vkDestroyShaderModule(p->r->log_dev, p->frag_shaders[i], NULL);
    }

    free(p->shader_stages);
    free(p->attr_descrs);
    free(p->binding_descrs);
    free(p->layout_bindings);
    free(p->texture_layout_bindings);
    free(p);
}

static void pipeline_free(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    struct RenderImpl* r = p->r;
    int i;

    vkDeviceWaitIdle(r->log_dev);

    if (p->owns) {
        for (i = 0; i < p->n_uniforms; i++) {
            p->b->destroy(p->b, p->uniforms[i].base.base.id);
        }
    }
    free(p->uniforms); p->uniforms = NULL; p->n_uniforms = 0;

    if (p->owns) {
        for (i = 0; i < p->n_buffers; i++) {
            p->b->destroy(p->b, p->buffers[i].base.base.id);
        }
    }
    free(p->buffers); p->n_buffers = 0;
    p->buffers = NULL;

    vkDestroyDescriptorPool(r->log_dev, p->descriptorPool, NULL);
    vkDestroyDescriptorSetLayout(r->log_dev, p->layout, NULL);
    if (p->n_samplers) {
        vkDestroyDescriptorSetLayout(r->log_dev, p->texLayout, NULL);
    }

    for (i = 0; i < p->n_samplers; i++) {
        vkDestroySampler(r->log_dev, p->samplers[i], NULL);
    }
    free(p->samplers); p->samplers = NULL; p->n_samplers = 0;

    free(p->buf_descr); p->n_buf_descr = 0; p->buf_descr = NULL;

    vkDestroyPipeline(r->log_dev, p->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(r->log_dev, p->pipelineLayout, NULL);
    free(p->descriptorSets); p->descriptorSets = NULL; p->n_descriptor_sets = 0;
    free(p->descriptorSetFlags); p->descriptorSetFlags = NULL;
    free(p);
}

static struct PipelineBuilder* enable_depth(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->enable_depth = 1;
    return p1;
}


static struct PipelineBuilder* enable_blend(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->enable_blend = 1;
    return p1;
}

static struct PipelineBuilder* enable_cull(struct PipelineBuilder* p1, enum CullType cull) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    switch (cull) {
    case CULL_FRONT:
        p->cull_mode = VK_CULL_MODE_FRONT_BIT;
        break;
    case CULL_BACK:
        p->cull_mode = VK_CULL_MODE_BACK_BIT;
        break;
    case CULL_BOTH:
        p->cull_mode = VK_CULL_MODE_FRONT_AND_BACK;
        break;
    default:
        assert(0);
        break;
    }
    return p1;
}

static struct PipelineBuilder* set_bmgr(struct PipelineBuilder* p1, struct BufferManager* b)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->b = b;
    return p1;
}

static struct PipelineBuilder* set_geometry(struct PipelineBuilder* p1, enum GeometryType geometry)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->geometry = geometry;
    return p1;
}

static struct Pipeline* build(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct PipelineImpl* pl = calloc(1, sizeof(*pl));
    struct RenderImpl* r = p->r;
    struct Pipeline base = {
        .buffer_assign = buffer_assign,
        .storage_swap = storage_swap,
        .uniform_update = uniform_update,
        .storage_assign = uniform_assign,
        .uniform_assign = uniform_assign,
        .buffer_update = buffer_update_,
        .buffer_create = buffer_create_,
        .free = pipeline_free,
        .start_compute = start_compute,
        .start = start,
        .use_texture = use_texture,
        .draw = draw
    };
    pl->base = base;
    pl->r = r;

    // TODO: check end state
    // Create samplers
    pl->samplers = malloc(p->n_samplers*sizeof(VkSampler));
    pl->n_samplers = p->n_samplers;
    for (int i = 0; i < p->n_samplers; i++) {
        if (vkCreateSampler(r->log_dev, &p->samplers[i].info, NULL, &pl->samplers[i]) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create texture sampler\n");
            exit(-1);
        }
    }

    // Vertex input State
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = p->n_buffers,
        .pVertexBindingDescriptions = get_binding_descrs(p),
        .vertexAttributeDescriptionCount = p->n_attrs,
        .pVertexAttributeDescriptions = get_attr_descrs(p)
    };
    // Vertex Input assembly State
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology =  primitive_topology(p->geometry),
        .primitiveRestartEnable = VK_FALSE
    };
    // Rasterization State
    VkPipelineRasterizationStateCreateInfo rastStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = p->cull_mode,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, //VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f
    };
    // Multisampling State
    VkPipelineMultisampleStateCreateInfo msStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    // Color Blend State
    VkPipelineColorBlendAttachmentState  cbAttach = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = p->enable_blend,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    };

    VkPipelineColorBlendStateCreateInfo cbCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &cbAttach
    };

    VkPipelineViewportStateCreateInfo vpStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = NULL,
        .scissorCount = 1,
        .pScissors = NULL
    };

    // Descriptor
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSetLayout textureDescriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = p->n_uniforms,
        .pBindings = get_layout_bindings(p)
    };

    if (vkCreateDescriptorSetLayout(r->log_dev, &layoutCreateInfo, NULL, &descriptorSetLayout) != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create descriptor set layout\n");
        exit(-1);
    }

    pl->layout = descriptorSetLayout;

    if (p->n_samplers) {
        VkDescriptorSetLayoutCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = p->n_samplers,
            .pBindings = get_texture_layout_bindings(p)
        };

        if (vkCreateDescriptorSetLayout(r->log_dev, &info, NULL, &textureDescriptorSetLayout) != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create descriptor set layout\n");
            exit(-1);
        }

        pl->texLayout = textureDescriptorSetLayout;
    }

    VkDescriptorPoolSize storagePool = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 10*r->sc.n_images
    };

    VkDescriptorPoolSize uniformPool = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 10*r->sc.n_images
    };

    VkDescriptorPoolSize samplerPool = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 100*r->sc.n_images
    };

    VkDescriptorPoolSize poolSizes[] = {
        storagePool, uniformPool, samplerPool
    };

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = sizeof(poolSizes)/sizeof(VkDescriptorPoolSize),
        .pPoolSizes = poolSizes,
        .maxSets = uniformPool.descriptorCount+samplerPool.descriptorCount
    };

    if (vkCreateDescriptorPool(r->log_dev, &poolInfo, NULL, &pl->descriptorPool) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create descriptor pool\n");
        exit(-1);
    }

    // create vector of 2 decriptorSetLayout with the layouts
    VkDescriptorSetLayout* layouts = malloc(r->sc.n_images*sizeof(VkDescriptorSetLayout));
    for (int i = 0; i < r->sc.n_images;i++) {
        layouts[i] = descriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pl->descriptorPool,
        .descriptorSetCount = r->sc.n_images,
        .pSetLayouts = layouts
    };

    pl->n_descriptor_sets = allocInfo.descriptorSetCount;
    pl->descriptorSets = malloc(pl->n_descriptor_sets*sizeof(VkDescriptorSet));
    pl->descriptorSetFlags = calloc(pl->n_descriptor_sets, sizeof(int));
    if (vkAllocateDescriptorSets(r->log_dev, &allocInfo, pl->descriptorSets) != VK_SUCCESS) {
        fprintf(stderr, "failed to allocate descriptor sets\n");
        exit(-1);
    }
    free(layouts);

    VkDescriptorSetLayout activeLayouts [] = {
        pl->layout, pl->texLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = p->n_samplers == 0 ? 1 : 2,
        .pSetLayouts = activeLayouts
    };

    if (vkCreatePipelineLayout(r->log_dev, &pipelineLayoutInfo, NULL, &pl->pipelineLayout) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create pieline layout\n");
        exit(-1);
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = p->enable_depth,
        .depthWriteEnable = p->enable_depth,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f, // Optional
        .maxDepthBounds = 1.0f, // Optional
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {}
    };

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pDynamicStates = dynamicStates,
        .dynamicStateCount = 2,
        .flags = 0
    };

    // Create Graphics Pipeline
    assert(p->n_comp_shaders * p->n_vert_shaders == 0);

    if (p->n_comp_shaders) {
        assert(p->n_comp_shaders == 1);

        VkComputePipelineCreateInfo compInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = *get_shader_stages(p),
            .layout = pl->pipelineLayout,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        if (vkCreateComputePipelines(r->log_dev, VK_NULL_HANDLE, 1, &compInfo, NULL, &pl->computePipeline) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create compute pipeline\n");
            exit(-1);
        }
    } else {
        VkGraphicsPipelineCreateInfo gpInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = p->n_comp_shaders+p->n_frag_shaders+p->n_vert_shaders,
            .pStages = get_shader_stages(p),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssemblyInfo,
            .pViewportState = &vpStateInfo,
            .pRasterizationState = &rastStateCreateInfo,
            .pMultisampleState = &msStateInfo,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &cbCreateInfo,
            .pDynamicState = &dynamicStateInfo,
            .layout = pl->pipelineLayout,
            .renderPass = r->rp.rp,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = -1
        };

        if (vkCreateGraphicsPipelines(r->log_dev, VK_NULL_HANDLE, 1, &gpInfo, NULL, &pl->graphicsPipeline) != VK_SUCCESS) {
            fprintf(stderr, "Failed to create graphics pipeline\n");
            exit(-1);
        }
    }

    pl->n_uniforms = p->n_uniforms;
    pl->uniforms = malloc(pl->n_uniforms*sizeof(struct UniformBlock));
    memcpy(pl->uniforms, p->uniforms, p->n_uniforms*sizeof(struct UniformBlock));

    pl->buffers = malloc(p->n_allocated*sizeof(struct Buffer));
    memcpy(pl->buffers, p->allocated, p->n_allocated*sizeof(struct Buffer));
    pl->n_buffers = pl->buf_cap = p->n_allocated;

    pl->buf_descr = malloc(p->n_buffers*sizeof(struct BufferDescriptor));
    memcpy(pl->buf_descr, p->buffers, p->n_buffers*sizeof(struct BufferDescriptor));
    pl->n_buf_descr = p->n_buffers;
    pl->b = p->b;
    pl->owns = p->owns;

    builder_free(p);

    return (struct Pipeline*)pl;
}

struct PipelineBuilder* pipeline_builder_vulkan(struct Render* r) {
    struct PipelineBuilderImpl* p = calloc(1, sizeof(*p));

    struct PipelineBuilder base = {
        .set_bmgr = set_bmgr,

        .storage_add = storage_add,
        .uniform_add = uniform_add,
        .begin_program = begin_program,
        .end_program = end_program,
        .add_vs = add_vs,
        .add_fs = add_fs,
        .add_cs = add_cs,

        .begin_buffer = begin_buffer,
        .buffer_attribute = buffer_attribute,
        .end_buffer = end_buffer,
        .enable_depth = enable_depth,
        .enable_blend = enable_blend,
        .enable_cull = enable_cull,
        .begin_sampler = begin_sampler,
        .end_sampler = end_sampler,
        .geometry = set_geometry,
        .build = build,
    };
    p->base = base;
    p->r = (struct RenderImpl*)r;
    p->geometry = GEOM_TRIANGLES;

    return (struct PipelineBuilder*)p;
}
