#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <render/pipeline.h>
#include <render/program.h>
#include <opengl/buffer.h>

struct UniformBlock {
    struct BufferImpl base;

    int id;
    const char* name;
    GLuint binding;
    GLuint index;
};

struct Buffer {
    struct BufferImpl base;

    int id;
    int n_vertices; // pipeline
    GLuint vao; // pipeline
    int stride; // pipeline
    int binding;  // pipeline
    int descriptor; // pipeline
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
    int buf_cap;

    struct BufferDescriptor* buf_descr;
    int n_buf_descrs;

    struct Sampler* samplers;
    int n_samplers;

    int enable_depth;
    int enable_blend;
    enum GeometryType geometry;

    struct BufferManager* b; // TODO: remove me
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
    int enable_blend;
    enum GeometryType geometry;

    struct BufferManager* b; // TODO: remove me
};

static struct PipelineBuilder* begin_uniform(
    struct PipelineBuilder*p1,
    int binding,
    const char* name,
    int size)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    assert(p->n_programs > 0);
    int program = prog_handle(p->programs[p->n_programs-1]);
    GLuint index = glGetUniformBlockIndex(program, name);
    glUniformBlockBinding(program, index, binding);

    struct BufferImpl base;
    if (!p->b) {
        p->b = buf_mgr_opengl_new(p->r); // TODO: remove me
    }
    int id = p->b->create(p->b, BUFFER_UNIFORM, MEMORY_DYNAMIC, NULL, size);
    memcpy(&base, p->b->get(p->b, id), sizeof(base));

    glBindBufferBase(GL_UNIFORM_BUFFER, binding, base.buffer);

    struct UniformBlock block = {
        .base = base,
        .id = id,
        .name = name,
        .binding = binding,
        .index = index,
    };
    p->cur_uniform = &p->uniforms[p->n_uniforms++];
    *p->cur_uniform = block;
    return p1;
}

static struct PipelineBuilder* uniform_add(
    struct PipelineBuilder*p1,
    int binding,
    const char* name)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    assert(p->n_programs > 0);
    int program = prog_handle(p->programs[p->n_programs-1]);
    GLuint index = glGetUniformBlockIndex(program, name);
    glUniformBlockBinding(program, index, binding);

    struct UniformBlock block = {
        .id = -1,
        .name = name,
        .binding = binding,
        .index = index,
    };
    p->uniforms[p->n_uniforms++] = block;
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
    assert(p->cur_program);
    prog_add_vs(p->cur_program, shader.glsl);
    return p1;
}

static struct PipelineBuilder* add_fs(struct PipelineBuilder*p1, struct ShaderCode shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    assert(p->cur_program);
    prog_add_fs(p->cur_program, shader.glsl);
    return p1;
}

static struct PipelineBuilder* add_cs(struct PipelineBuilder*p1, struct ShaderCode shader) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    assert(p->cur_program);
    prog_add_cs(p->cur_program, shader.glsl);
    return p1;
}

static struct PipelineBuilder* end_program(struct PipelineBuilder*p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    assert(p->cur_program);
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
        p->b->destroy(p->b, p->buffers[i].id); // TODO: remove me
        if (p->buffers[i].vao) {
            glDeleteVertexArrays(1, &p->buffers[i].vao);
        }
    }
    free(p->buffers);
    for (i = 0; i < p->n_uniforms; i++) {
        p->b->destroy(p->b, p->uniforms[i].id);
    }
    free(p->uniforms);
    for (i = 0; i < p->n_samplers; i++) {
        glDeleteSamplers(1, &p->samplers[i].id);
    }
    free(p->samplers);
    free(p->buf_descr);
    free(p);
}

static void uniform_assign(struct Pipeline* p1, int uniform_id, int buffer_id)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(uniform_id < p->n_uniforms);
    p->uniforms[uniform_id].id = buffer_id;
    memcpy(&p->uniforms[uniform_id].base,
           p->b->get(p->b, buffer_id),
           sizeof(p->uniforms[uniform_id].base));
}

static void uniform_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(id < p->n_uniforms);
    p->b->update(p->b, p->uniforms[id].id, data, offset, size);
}

// TODO: remove
static void buffer_update(struct Pipeline* p1, int id, const void* data, int offset, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    // TODO: check is dynamic
    assert(id < p->n_buffers);
    p->b->update(p->b, p->buffers[id].id, data, offset, size);
}

static int buffer_storage_create(
    struct Pipeline* p1,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    int binding, int descriptor,
    const void* data, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(descriptor < p->n_buf_descrs);
    struct BufferDescriptor* descr = descriptor < 0 ? NULL : &p->buf_descr[descriptor];
    if (p->n_buffers >= p->buf_cap) {
        p->buf_cap = (p->buf_cap+1)*2;
        p->buffers = realloc(p->buffers, p->buf_cap*sizeof(struct Buffer));
    }
    int buffer_id = p->n_buffers++; // logical id
    struct Buffer* buf = &p->buffers[buffer_id];
    memset(buf, 0, sizeof(*buf));
    buf->id = p->b->create(p->b, type, mem_type, data, size);
    memcpy(&buf->base, p->b->get(p->b, buf->id), sizeof(buf->base));

    buf->stride = descr ? descr->stride : 0;
    buf->n_vertices = descr ? size / descr->stride : 0;
    buf->descriptor = descriptor;
    buf->binding = binding;

    if (binding >= 0) {
        glBindBufferBase(buf->base.type, binding, buf->base.buffer);
        //glBufferData(btype, buf->size, data, mtype);
    }

    if (descr) {
        glBindBuffer(GL_ARRAY_BUFFER, buf->base.buffer);
        glGenVertexArrays(1, &buf->vao);
        glBindVertexArray(buf->vao);

        for (int j = 0; j < descr->n_attrs; j++) {
            int location = descr->attrs[j].location;
            glEnableVertexAttribArray(location);
            glVertexAttribPointer(
                location, descr->attrs[j].channels, GL_FLOAT, GL_FALSE,
                descr->stride,
                (const void*)descr->attrs[j].offset);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    return buffer_id;
}

// TODO: maybe buffer_vertex_create and remove type?
static int buffer_create(
    struct Pipeline* p1,
    enum BufferType type,
    enum BufferMemoryType mem_type,
    int binding, const void* data, int size)
{
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(binding < p->n_buf_descrs);
    struct BufferDescriptor* descr = &p->buf_descr[binding];
    if (p->n_buffers >= p->buf_cap) {
        p->buf_cap = (p->buf_cap+1)*2;
        p->buffers = realloc(p->buffers, p->buf_cap*sizeof(struct Buffer));
    }
    int buffer_id = p->n_buffers++;
    struct Buffer* buf = &p->buffers[buffer_id];
    memset(buf, 0, sizeof(*buf));
    buf->id = p->b->create(p->b, type, mem_type, data, size);
    memcpy(&buf->base, p->b->get(p->b, buf->id), sizeof(buf->base));

    buf->stride = descr->stride;
    buf->n_vertices = size / descr->stride;
    buf->descriptor = binding;

    glBindBuffer(GL_ARRAY_BUFFER, buf->base.buffer);
    glGenVertexArrays(1, &buf->vao);
    glBindVertexArray(buf->vao);

    for (int j = 0; j < descr->n_attrs; j++) {
        int location = descr->attrs[j].location;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location, descr->attrs[j].channels, GL_FLOAT, GL_FALSE,
            descr->stride,
            (const void*)descr->attrs[j].offset);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return buffer_id;
}

void buffer_copy(struct Pipeline* p1, int dst, int src) {
//    struct PipelineImpl* p = (struct PipelineImpl*)p1;
//    assert(dst < p->n_buffers);
//    assert(src < p->n_buffers);
//    struct Buffer* src_buf = &p->buffers[src];
//    struct Buffer* dst_buf = &p->buffers[dst];

    assert(0);

//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, src_buf->vbo);
//    glBindBuffer(GL_ARRAY_BUFFER, dst_buf->vbo);
//    glCopyBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_ARRAY_BUFFER, 0, 0, src_buf->size);
}


void buffer_swap(struct Pipeline* p1, int dst, int src) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(dst < p->n_buffers);
    assert(src < p->n_buffers);
    struct Buffer* src_buf = &p->buffers[src];
    struct Buffer* dst_buf = &p->buffers[dst];
    assert(src_buf->binding >= 0);
    assert(dst_buf->binding >= 0);

    glBindBufferBase(src_buf->base.type, dst_buf->binding, src_buf->base.buffer);
    glBindBufferBase(dst_buf->base.type, src_buf->binding, dst_buf->base.buffer);

    int t = src_buf->binding;
    src_buf->binding = dst_buf->binding;
    dst_buf->binding = t;
}

static void before(struct PipelineImpl* p) {
    if (p->enable_blend) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if (p->enable_depth) {
        glEnable(GL_DEPTH_TEST);
    }
    if (p->geometry == GEOM_POINTS) {
        glEnable(GL_PROGRAM_POINT_SIZE);
    }
}

static void after(struct PipelineImpl* p) {
    if (p->enable_blend) {
        glDisable(GL_BLEND);
    }
    if (p->enable_depth) {
        glDisable(GL_DEPTH_TEST);
    }
    if (p->geometry == GEOM_POINTS) {
        glDisable(GL_PROGRAM_POINT_SIZE);
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
            p->uniforms[i].base.buffer);
    }
}

static void start_part(struct Pipeline* p1, int part) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;

    assert(part < p->n_programs);
    prog_use(p->programs[part]);

    // TODO: what samplers to use?
    for (i = 0; i < p->n_samplers; i++) {
        glBindSampler(p->samplers[i].binding, p->samplers[i].id);
    }

    // TODO: what uniforms to use?
    for (i = 0; i < p->n_uniforms; i++) {
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            p->uniforms[i].binding,
            p->uniforms[i].base.buffer);
    }
}

static void start_compute_part(struct Pipeline* p1, int part, int sx, int sy, int sz) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    //int i;

    //printf("compute %d %d %d %d\n", part, sx, sy, sz);
    assert(part < p->n_programs);
    prog_use(p->programs[part]);

    static int a = 0;
    static int b = 3;

    // TODO: bind
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, p->buffers[a].vbo);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, p->buffers[b].vbo);

    // TODO: what uniforms to use?
    /*for (i = 0; i < p->n_uniforms; i++) {
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            p->uniforms[i].binding,
            p->uniforms[i].buffer);
    }*/

    glDispatchCompute(sx, sy, sz);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // TODO

    int t = a; a = b; b = t;
}

static void wait_part(struct Pipeline* p1, int part) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(part < p->n_programs);
    // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // TODO
}

static void use_texture(struct Pipeline* p1, void* tex) {
    //struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int tex_id = *(int*)tex;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);
}

static void draw(struct Pipeline* p1, int id) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    before(p);
    glBindVertexArray(p->buffers[id].vao);
    if (p->geometry == GEOM_POINTS) {
        glDrawArrays(GL_POINTS, 0, p->buffers[id].n_vertices);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, p->buffers[id].n_vertices);
    }

    after(p);
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

static struct PipelineBuilder* geometry(struct PipelineBuilder* p1, enum GeometryType type) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->geometry = type;
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

struct PipelineBuilder* set_bmgr(struct PipelineBuilder* p1, struct BufferManager* b)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->b = b;
    return p1;
}

static struct Pipeline* build(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct PipelineImpl* pl = calloc(1, sizeof(*pl));
    struct Pipeline base = {
        .free = pipeline_free,
        .uniform_assign = uniform_assign,
        .uniform_update = uniform_update,
        .buffer_copy = buffer_copy,
        .buffer_update = buffer_update,
        .buffer_create = buffer_create,
        .buffer_storage_create = buffer_storage_create,
        .buffer_swap = buffer_swap,
        .start = start,
        .start_part = start_part,
        .start_compute_part = start_compute_part,
        .wait_part = wait_part,
        .draw = draw,
        .use_texture = use_texture
    };
    pl->base = base;
    pl->n_programs = p->n_programs;
    pl->programs = malloc(pl->n_programs*sizeof(struct Program*));
    memcpy(pl->programs, p->programs, pl->n_programs*sizeof(struct Program*));

    pl->n_uniforms = p->n_uniforms;
    pl->uniforms = malloc(pl->n_uniforms*sizeof(struct UniformBlock));
    memcpy(pl->uniforms, p->uniforms, pl->n_uniforms*sizeof(struct UniformBlock));

    pl->n_buf_descrs = p->n_buffers;
    pl->buf_descr = malloc(pl->n_buf_descrs*sizeof(struct BufferDescriptor));
    memcpy(pl->buf_descr, p->buffers, pl->n_buf_descrs*sizeof(struct BufferDescriptor));

    pl->n_samplers = p->n_samplers;
    pl->samplers = malloc(pl->n_samplers*sizeof(struct Sampler));
    memcpy(pl->samplers, p->samplers, pl->n_samplers*sizeof(struct Sampler));

    pl->enable_depth = p->enable_depth;
    pl->enable_blend = p->enable_blend;
    pl->geometry = p->geometry;
    pl->b = p->b;

    free(p);
    return (struct Pipeline*)pl;
}

struct PipelineBuilder* pipeline_builder_opengl(struct Render* r) {
    struct PipelineBuilderImpl* p = calloc(1, sizeof(*p));
    struct PipelineBuilder base = {
        .set_bmgr = set_bmgr,

        .begin_program = begin_program,
        .add_vs = add_vs,
        .add_fs = add_fs,
        .add_cs = add_cs,
        .end_program = end_program,

        .uniform_add = uniform_add,
        .begin_uniform = begin_uniform,
        .end_uniform = end_uniform,

        .begin_buffer = begin_buffer,
        .buffer_attribute = buffer_attribute,
        .end_buffer = end_buffer,

        .begin_sampler = begin_sampler,
        .end_sampler = end_sampler,

        .enable_depth = enable_depth,
        .enable_blend = enable_blend,

        .geometry = geometry,

        .build = build
    };
    p->base = base;
    p->r = r;
    return (struct PipelineBuilder*)p;
}
