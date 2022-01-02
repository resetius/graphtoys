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

    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSet descriptorSet;
};

static void update_uniform(struct Triangle* tr, struct DrawContext* ctx) {
    int width = tr->r->sc.extent.width;
    int height = tr->r->sc.extent.height;
    float ratio = width / (float)height;
    //float ratio = 1.0;
//printf("ratio = %f\n", ratio);
    mat4x4 m, p, mvp;
    mat4x4_identity(m);
    //m[1][1] = -1;
    //m[3][1] = 3;
    mat4x4_rotate_Z(m, m, ctx->time);
    // flipped y
    //mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
    mat4x4_mul(mvp, p, m);
    //mvp[1][1] *= -1;
    //mvp[3][1] += 1;

    void* data;
    vkMapMemory(tr->r->log_dev, tr->uniformBufferMemory, 0, sizeof(mat4x4), 0, &data);

    memcpy(data, mvp, sizeof(mat4x4));

    vkUnmapMemory(tr->r->log_dev, tr->uniformBufferMemory);
}

static void trvk_draw(struct Object* obj, struct DrawContext* ctx) {
    struct Triangle* tr = (struct Triangle*)obj;
    struct RenderImpl* r = tr->r;

    VkCommandBuffer cBuffer = r->buffer;

    update_uniform(tr, ctx);

    vkCmdBindPipeline(cBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, tr->graphicsPipeline);

    VkBuffer vertexBuffers[] = { tr->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };

    vkCmdBindVertexBuffers(
        cBuffer,
        0,
        1,
        vertexBuffers,
        offsets);

	//	Bind uniform buffer using descriptorSets
	vkCmdBindDescriptorSets(
        cBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        tr->pipelineLayout,
        0,
        1,
        &tr->descriptorSet, 0, NULL);

    vkCmdDraw(
        cBuffer,
        3, // vertices
        1, // instance count -- just the 1
        0, // vertex offet -- any offsets to add
        0);// first instance -- since no instancing, is set to 0
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
        .offset = offsetof(struct Vertex, col)
    };
    attributeDescriptions[0] = posDescr;
    VkVertexInputAttributeDescription colDescr = {
        .binding = 0,
        .location = 1, // vPos
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(struct Vertex, pos)
    };
    attributeDescriptions[1] = colDescr;
    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = sizeof(struct Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

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
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &tr->vertexBuffer,
        &tr->vertexBufferMemory);
    // copybuffer
    copy_buffer(r->graphics_family, r->g_queue, r->log_dev, stagingBuffer, tr->vertexBuffer, bufferSize);

    vkDestroyBuffer(r->log_dev, stagingBuffer, NULL);
	vkFreeMemory(r->log_dev, stagingBufferMemory, NULL);
    printf("Vertex buffer created\n");

    VkDeviceSize uboBufferSize = sizeof(mat4x4);
    create_buffer(
        r->phy_dev, r->log_dev,
        uboBufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &tr->uniformBuffer,
        &tr->uniformBufferMemory);

    // descriptor

    VkDescriptorPoolSize poolSizes = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = r->sc.n_images
    };

	VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSizes,
        .maxSets = r->sc.n_images
    };

    VkDescriptorPool descriptorPool;
	if (vkCreateDescriptorPool(r->log_dev, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
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
        .descriptorPool = descriptorPool,
        .descriptorSetCount = r->sc.n_images,
        .pSetLayouts = layouts
    };

	if (vkAllocateDescriptorSets(r->log_dev, &allocInfo, &tr->descriptorSet) != VK_SUCCESS) {
		fprintf(stderr, "failed to allocate descriptor sets\n");
        exit(-1);
	}
    free(layouts);

    for (int i = 0; i < r->sc.n_images; i++) {
        VkDescriptorBufferInfo uboBufferDescInfo = {
            .buffer = tr->uniformBuffer,
            .offset = 0,
            .range = sizeof(uboBufferSize)
        };

        VkWriteDescriptorSet uboDescWrites = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = NULL,
            .dstSet = tr->descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &uboBufferDescInfo,
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptorSetLayout
    };

	if (vkCreatePipelineLayout(r->log_dev, &pipelineLayoutInfo, NULL, &tr->pipelineLayout) != VK_SUCCESS) {
		fprintf(stderr, "Failed to create pieline layout\n");
        exit(-1);
	}

    VkPipelineShaderStageCreateInfo vertShaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShader,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo fragShaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShader,
        .pName = "main"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageCreateInfo, fragShaderStageCreateInfo };

    // Vertex input State
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    // Vertex Input assembly State
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology =  VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Rasterization State
	VkPipelineRasterizationStateCreateInfo rastStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE, // VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
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
        .blendEnable = VK_FALSE
    };

	VkPipelineColorBlendStateCreateInfo cbCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &cbAttach
    };

    // Viewport State Create Info

    int w = r->sc.extent.width;
    int h = r->sc.extent.height;
	VkViewport viewport = {
        .x = 0,
        .y = h,
        .width = w,
        .height = -h,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = { 0,0 },
        .extent = r->sc.extent
    };

	VkPipelineViewportStateCreateInfo vpStateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    // Create Graphics Pipeline

	VkGraphicsPipelineCreateInfo gpInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssemblyInfo,
        .pViewportState = &vpStateInfo,
        .pRasterizationState = &rastStateCreateInfo,
        .pMultisampleState = &msStateInfo,
        .pDepthStencilState = NULL,
        .pColorBlendState = &cbCreateInfo,
        .pDynamicState = NULL,
        .layout = tr->pipelineLayout,
        .renderPass = r->rp.rp,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if (vkCreateGraphicsPipelines(r->log_dev, VK_NULL_HANDLE, 1, &gpInfo, NULL, &tr->graphicsPipeline) != VK_SUCCESS) {
        fprintf(stderr, "Failed to create graphics pipeline\n");
        exit(-1);
	}
    printf("Pipeline loaded\n");

    return (struct Object*)tr;
}
