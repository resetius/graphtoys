#include <stdlib.h>
#include <stdio.h>

#include <render/pipeline.h>
#include <render/program.h>

#include "glad/gl.h"

struct UniformBlock {
    const char* name;
    GLuint binding;
    GLuint buffer;
    GLuint index;
    int size;
};

struct Buffer {
    int n_vertices;
    GLuint vbo;
    GLuint vao;
    int size;
    int dynamic;
    int stride;
};

struct BufferAttribute {
    int location;
    int channels;
    int bytes_per_channel;
    int stride;
    uint64_t offset;
};

struct BufferDescriptor {
    int stride;
    int dynamic;
    struct BufferAttribute attrs[10];
    int n_attrs;
    const void* data;
    int size;
};

struct Sampler {
    GLuint id;
    int binding;
};

struct PipelineImpl {
    struct Pipeline base;

    struct Render* r;
    struct Program** programs;
    int n_programs;

    struct UniformBlock* uniforms;
    int n_uniforms;

    struct Buffer* buffers;
    int n_buffers;

    struct BufferDescriptor* buf_descr;
    int n_buf_descrs;

    struct Sampler* samplers;
    int n_samplers;

    int enable_depth;
};

struct PipelineBuilderImpl {
    struct PipelineBuilder base;
    struct Render* r;

    struct Program* programs[100];
    struct Program* cur_program;
    int n_programs;

    struct UniformBlock uniforms[100];
    struct UniformBlock* cur_uniform;
    int n_uniforms;

    //struct Buffer buffers[100];
    //struct Buffer* cur_buffer;
    struct BufferDescriptor buffers[100];
    struct BufferDescriptor* cur_buffer;
    int n_buffers;

    struct Sampler samplers[100];
    struct Sampler* cur_sampler;
    int n_samplers;

    int enable_depth;
};

static struct PipelineBuilder* begin_uniform(
    struct PipelineBuilder*p1,
    int binding,
    const char* name,
    int size)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check program
    int program = prog_handle(p->programs[p->n_programs-1]);
    GLuint index = glGetUniformBlockIndex(program, name);
    GLuint buffer;
    glUniformBlockBinding(program, index, binding);

    glGenBuffers(1, &buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);

    struct UniformBlock block = {
        .name = name,
        .binding = binding,
        .index = index,
        .buffer = buffer,
        .size = size
    };
    p->cur_uniform = &p->uniforms[p->n_uniforms++];
    *p->cur_uniform = block;
    return p1;
}

static struct PipelineBuilder* end_uniform(struct PipelineBuilder *p1)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_uniform = NULL;
    return p1;
}

static struct PipelineBuilder* begin_program(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_program = p->programs[p->n_programs++] = rend_prog_new(p->r);
    return p1;
}

static struct PipelineBuilder* add_vs(struct PipelineBuilder*p1, struct ShaderCode shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_add_vs(p->cur_program, shader.glsl);
    return p1;
}

static struct PipelineBuilder* add_fs(struct PipelineBuilder*p1, struct ShaderCode shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_add_fs(p->cur_program, shader.glsl);
    return p1;
}

static struct PipelineBuilder* end_program(struct PipelineBuilder*p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_link(p->cur_program);
    p->cur_program = NULL;
    return p1;
}

static struct PipelineBuilder* begin_buffer(struct PipelineBuilder* p1, int stride) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check current
    p->cur_buffer = &p->buffers[p->n_buffers++];
    p->cur_buffer->stride = stride;

    return p1;
}

static struct PipelineBuilder* buffer_data(struct PipelineBuilder* p1, const void* data, int size)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check current
    p->cur_buffer->size = size;
    p->cur_buffer->data = data;
    return p1;
}

static struct PipelineBuilder* buffer_dynamic(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check current
    p->cur_buffer->dynamic = 1;
    return p1;
}

static struct PipelineBuilder* buffer_attribute(
    struct PipelineBuilder* p1,
    int location,
    int channels, int bytes_per_channel,
    uint64_t offset)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct BufferAttribute* attr = &p->cur_buffer->attrs[p->cur_buffer->n_attrs++];
    attr->location = location;
    attr->channels = channels;
    attr->bytes_per_channel = bytes_per_channel;
    attr->stride = p->cur_buffer->stride;
    attr->offset = offset;
    return p1;
}

static struct PipelineBuilder* end_buffer(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_buffer = NULL;
    return p1;
}

static void pipeline_free(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;
    if (!p) {
        return;
    }
    for (i = 0; i < p->n_programs; i++) {
        prog_free(p->programs[i]);
    }
    free(p->programs);
    for (i = 0; i < p->n_buffers; i++) {
        glDeleteBuffers(1, &p->buffers[i].vbo);
        if (p->buffers[i].vao) {
            glDeleteVertexArrays(1, &p->buffers[i].vao);
        }
    }
    free(p->buffers);
    for (i = 0; i < p->n_uniforms; i++) {
        glDeleteBuffers(1, &p->uniforms[i].buffer);
    }
    free(p->uniforms);
    for (i = 0; i < p->n_samplers; i++) {
        glDeleteSamplers(1, &p->samplers[i].id);
    }
    free(p->samplers);
    free(p->buf_descr);
    free(p);
}

static void uniform_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    // check id < n_buffers
    glBindBuffer(GL_ARRAY_BUFFER, p->uniforms[id].buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void buffer_update(struct Pipeline* p1, int id, int i, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    // check id < n_buffers
    int j;

    if (id >= p->n_buffers) {
        p->buffers = realloc(p->buffers, (id+1)*sizeof(struct Buffer));
        memset(p->buffers+p->n_buffers, 0, (id-p->n_buffers+1)* sizeof(struct Buffer));
        p->n_buffers = id+1;
    }

    struct Buffer* buf = &p->buffers[id];
    if (buf->size > 0 && buf->size != size) {
        glDeleteBuffers(1, &buf->vbo);
    }
    if (buf->size != size) {
        buf->n_vertices = size / p->buf_descr[i].stride;
        buf->size = size;

        glGenBuffers(1, &buf->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
        glBufferData(GL_ARRAY_BUFFER, buf->size, NULL, GL_DYNAMIC_DRAW);

        glGenVertexArrays(1, &buf->vao);
        glBindVertexArray(buf->vao);

        for (j = 0; j < p->buf_descr[i].n_attrs; j++) {
            int location = p->buf_descr[i].attrs[j].location;
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(
                location, p->buf_descr[i].attrs[j].channels, GL_FLOAT, GL_FALSE,
                p->buf_descr[i].stride,
                (const void*)p->buf_descr[i].attrs[j].offset);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, p->buffers[id].vbo);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

static void run(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;

    for (i = 0; i < p->n_programs; i++) {
        prog_use(p->programs[i]);
    }

    for (i = 0; i < p->n_samplers; i++) {
        glBindSampler(p->samplers[i].binding, p->samplers[i].id);
    }

    for (i = 0; i < p->n_uniforms; i++) {
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            p->uniforms[i].binding,
            p->uniforms[i].buffer);
    }

    for (i = 0; i < p->n_buffers; i++) {
        glBindVertexArray(p->buffers[i].vao);
        if (p->buffers[i].n_vertices) {
            glDrawArrays(GL_TRIANGLES, 0, p->buffers[i].n_vertices);
        }
    }
}

static void start(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;

    for (i = 0; i < p->n_programs; i++) {
        prog_use(p->programs[i]);
    }

    for (i = 0; i < p->n_samplers; i++) {
        glBindSampler(p->samplers[i].binding, p->samplers[i].id);
    }

    for (i = 0; i < p->n_uniforms; i++) {
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            p->uniforms[i].binding,
            p->uniforms[i].buffer);
    }
}

static void use_texture(struct Pipeline* p1, void* tex) {
    //struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int tex_id = *(int*)tex;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);
}

static void draw(struct Pipeline* p1, int id) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    glBindVertexArray(p->buffers[id].vao);
    glDrawArrays(GL_TRIANGLES, 0, p->buffers[id].n_vertices);
}

static struct PipelineBuilder* enable_depth(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->enable_depth = 1;
    return p1;
}

static struct PipelineBuilder* begin_sampler(struct PipelineBuilder* p1, int binding) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_sampler = &p->samplers[p->n_samplers++];
    p->cur_sampler->binding = binding;
    glGenSamplers(1, &p->cur_sampler->id);
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    vec4 border = {0.0f,0.0f,0.0f,0.5f};
    glSamplerParameterfv(p->cur_sampler->id, GL_TEXTURE_BORDER_COLOR, border);

    return p1;
}

static struct PipelineBuilder* end_sampler(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_sampler = NULL;
    return p1;
}

static struct Pipeline* build(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct PipelineImpl* pl = calloc(1, sizeof(*pl));
    struct Pipeline base = {
        .free = pipeline_free,
        .uniform_update = uniform_update,
        .buffer_update = buffer_update,
        .run = run,
        .start = start,
        .draw = draw,
        .use_texture = use_texture
    };
    int i, j, k;
    pl->base = base;
    pl->n_programs = p->n_programs;
    pl->programs = malloc(pl->n_programs*sizeof(struct Program*));
    memcpy(pl->programs, p->programs, pl->n_programs*sizeof(struct Program*));

    pl->n_uniforms = p->n_uniforms;
    pl->uniforms = malloc(pl->n_uniforms*sizeof(struct UniformBlock));
    memcpy(pl->uniforms, p->uniforms, pl->n_uniforms*sizeof(struct UniformBlock));

    for (i = 0; i < p->n_buffers; i++) {
        if (p->buffers[i].data) {
            pl->n_buffers ++;
        }
    }
    pl->buffers = malloc(pl->n_buffers*sizeof(struct Buffer));
    for (k = 0, i = 0; i < p->n_buffers; i++) {
        if (p->buffers[i].data) {
            struct Buffer* buf = &pl->buffers[k++];
            buf->n_vertices = p->buffers[i].size / p->buffers[i].stride;
            buf->size = p->buffers[i].size;

            printf("Create buffer: %d %d %d\n", buf->size, buf->n_vertices,
                   p->buffers[i].n_attrs);

            glGenBuffers(1, &buf->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);
            glBufferData(GL_ARRAY_BUFFER, buf->size,
                         p->buffers[i].data, GL_STATIC_DRAW);

            glGenVertexArrays(1, &buf->vao);
            glBindVertexArray(buf->vao);

            for (j = 0; j < p->buffers[i].n_attrs; j++) {
                int location = p->buffers[i].attrs[j].location;
                glEnableVertexAttribArray(location);
                glVertexAttribPointer(
                    location, p->buffers[i].attrs[j].channels, GL_FLOAT, GL_FALSE,
                    p->buffers[i].stride,
                    (const void*)p->buffers[i].attrs[j].offset);
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }

    pl->n_buf_descrs = p->n_buffers;
    pl->buf_descr = malloc(pl->n_buf_descrs*sizeof(struct BufferDescriptor));
    memcpy(pl->buf_descr, p->buffers, pl->n_buf_descrs*sizeof(struct BufferDescriptor));

    pl->n_samplers = p->n_samplers;
    pl->samplers = malloc(pl->n_samplers*sizeof(struct Sampler));
    memcpy(pl->samplers, p->samplers, pl->n_samplers*sizeof(struct Sampler));

    free(p);
    return (struct Pipeline*)pl;
}

struct PipelineBuilder* pipeline_builder_opengl(struct Render* r) {
    struct PipelineBuilderImpl* p = calloc(1, sizeof(*p));
    struct PipelineBuilder base = {
        .begin_program = begin_program,
        .add_vs = add_vs,
        .add_fs = add_fs,
        .end_program = end_program,

        .begin_uniform = begin_uniform,
        .end_uniform = end_uniform,

        .begin_buffer = begin_buffer,
        .buffer_data = buffer_data,
        .buffer_dynamic = buffer_dynamic,
        .buffer_attribute = buffer_attribute,
        .end_buffer = end_buffer,

        .begin_sampler = begin_sampler,
        .end_sampler = end_sampler,

        .enable_depth = enable_depth,

        .build = build
    };
    p->base = base;
    p->r = r;
    return (struct PipelineBuilder*)p;
}
