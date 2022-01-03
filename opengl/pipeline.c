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

    struct Buffer buffers[100];
    struct Buffer* cur_buffer;
    int n_buffers;
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

static struct PipelineBuilder* add_vs(struct PipelineBuilder*p1, const char* shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_add_vs(p->cur_program, shader);
    return p1;
}

static struct PipelineBuilder* add_fs(struct PipelineBuilder*p1, const char* shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_add_fs(p->cur_program, shader);
    return p1;
}

static struct PipelineBuilder* end_program(struct PipelineBuilder*p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check begin
    prog_link(p->cur_program);
    p->cur_program = NULL;
    return p1;
}

static struct PipelineBuilder* begin_buffer(struct PipelineBuilder* p1, int n_vertices) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check current
    p->cur_buffer = &p->buffers[p->n_buffers++];
    p->cur_buffer->n_vertices = n_vertices;

    glGenBuffers(1, &p->cur_buffer->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p->cur_buffer->vbo);

    return p1;
}

static struct PipelineBuilder* buffer_data(struct PipelineBuilder* p1, const void* data, int size)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    // TODO: check current
    p->cur_buffer->size = size;
    glBufferData(GL_ARRAY_BUFFER, size, data, p->cur_buffer->dynamic
                 ? GL_DYNAMIC_DRAW
                 : GL_STATIC_DRAW);
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
    int stride, uint64_t offset)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    if (!p->cur_buffer->vao) {
        glGenVertexArrays(1, &p->cur_buffer->vao);
        glBindVertexArray(p->cur_buffer->vao);
    }
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, channels, GL_FLOAT, GL_FALSE,
                          stride, (const void*)offset);

    return p1;
}

static struct PipelineBuilder* end_buffer(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_buffer = 0;
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
    free(p);
}

static void uniform_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    // check id < n_buffers
    glBindBuffer(GL_ARRAY_BUFFER, p->uniforms[id].buffer);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

static void buffer_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    // check id < n_buffers
    glBindBuffer(GL_ARRAY_BUFFER, p->buffers[id].vbo);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
}

static void run(struct Pipeline* p1) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;

    for (i = 0; i < p->n_programs; i++) {
        prog_use(p->programs[i]);
    }

    for (i = 0; i < p->n_buffers; i++) {
        glBindVertexArray(p->buffers[i].vao);
        glDrawArrays(GL_TRIANGLES, 0, p->buffers[i].n_vertices);
    }
}

static struct Pipeline* build(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct PipelineImpl* pl = calloc(1, sizeof(*pl));
    struct Pipeline base = {
        .free = pipeline_free,
        .uniform_update = uniform_update,
        .buffer_update = buffer_update,
        .run = run
    };
    pl->base = base;
    pl->n_programs = p->n_programs;
    pl->programs = malloc(pl->n_programs*sizeof(struct Program*));
    memcpy(pl->programs, p->programs, pl->n_programs*sizeof(struct Program*));

    pl->n_uniforms = p->n_uniforms;
    pl->uniforms = malloc(pl->n_uniforms*sizeof(struct UniformBlock));
    memcpy(pl->uniforms, p->uniforms, pl->n_uniforms*sizeof(struct UniformBlock));

    pl->n_buffers = p->n_buffers;
    pl->buffers = malloc(pl->n_buffers*sizeof(struct Buffer));
    memcpy(pl->buffers, p->buffers, pl->n_buffers*sizeof(struct Buffer));
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

        .build = build
    };
    p->base = base;
    p->r = r;
    return (struct PipelineBuilder*)p;
}
