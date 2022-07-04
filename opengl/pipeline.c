#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <render/pipeline.h>
#include <opengl/program.h>
#include <opengl/buffer.h>

#include <lib/verify.h>

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
    enum DataType data_type;
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

    int enable_cull;
    GLenum cull_mode;
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

    int enable_cull;
    GLenum cull_mode;
};

static int is_integer(enum DataType data_type) {
    int result = 0;
    switch (data_type) {
    case DATA_FLOAT:
        result = 0;
        break;
    case DATA_INT:
        result = 1;
        break;
    default:
        assert(0);
        break;
    }
    return result;
}

static GLint gl_data_type(enum DataType data_type) {
    GLint result = -1;
    switch (data_type) {
    case DATA_FLOAT:
        result = GL_FLOAT;
        break;
    case DATA_INT:
        result = GL_INT;
        break;
    default:
        assert(0);
        break;
    }
    return result;
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

static struct PipelineBuilder* storage_add(
    struct PipelineBuilder*p1,
    int binding,
    const char* name)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;

    struct UniformBlock block = {
        .id = -1,
        .name = name,
        .binding = binding,
    };
    p->uniforms[p->n_uniforms++] = block;
    return p1;
}

static struct PipelineBuilder* begin_program(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_program = p->programs[p->n_programs++] = prog_opengl_new(p->r);
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
    int channels,
    enum DataType data_type,
    uint64_t offset)
{
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    struct BufferAttribute* attr = &p->cur_buffer->attrs[p->cur_buffer->n_attrs++];
    attr->location = location;
    attr->channels = channels;
    attr->data_type = data_type;
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
        if (p->buffers[i].vao) {
            glDeleteVertexArrays(1, &p->buffers[i].vao);
        }
    }
    free(p->buffers);
    free(p->uniforms);
    for (i = 0; i < p->n_samplers; i++) {
        glDeleteSamplers(1, &p->samplers[i].id);
    }
    free(p->samplers);
    free(p->buf_descr);
    free(p);
}

static int buffer_assign(struct Pipeline* p1, int id, int buffer_id) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(id < p->n_buf_descrs);
    struct BufferDescriptor* descr = &p->buf_descr[id];

    if (p->n_buffers >= p->buf_cap) {
        p->buf_cap = (p->buf_cap+1)*2;
        p->buffers = realloc(p->buffers, p->buf_cap*sizeof(struct Buffer));
    }
    int logical_id = p->n_buffers++; // logical id
    struct Buffer* buf = &p->buffers[logical_id];
    memset(buf, 0, sizeof(*buf));
    buf->id = buffer_id;
    memcpy(&buf->base, p->b->get(p->b, buf->id), sizeof(buf->base));

    buf->stride = descr->stride;
    buf->n_vertices = buf->base.size / descr->stride;
    buf->descriptor = id;

    glBindBuffer(GL_ARRAY_BUFFER, buf->base.buffer);
    glGenVertexArrays(1, &buf->vao);
    glBindVertexArray(buf->vao);

    for (int j = 0; j < descr->n_attrs; j++) {
        int location = descr->attrs[j].location;
        glEnableVertexAttribArray(location);

        if (is_integer(descr->attrs[j].data_type)) {
            glVertexAttribIPointer(
                location,
                descr->attrs[j].channels,
                gl_data_type(descr->attrs[j].data_type),
                descr->stride,
                (const void*)descr->attrs[j].offset);
        } else {
            glVertexAttribPointer(
                location,
                descr->attrs[j].channels,
                gl_data_type(descr->attrs[j].data_type),
                GL_FALSE,
                descr->stride,
                (const void*)descr->attrs[j].offset);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return logical_id;
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

static void storage_swap(struct Pipeline* p1, int dst, int src) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    assert(dst < p->n_uniforms);
    assert(src < p->n_uniforms);
    struct UniformBlock* src_buf = &p->uniforms[src];
    struct UniformBlock* dst_buf = &p->uniforms[dst];

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
    if (p->enable_cull) {
        glEnable(GL_CULL_FACE);
        glCullFace(p->cull_mode);
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
    if (p->enable_cull) {
        glDisable(GL_CULL_FACE);
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
            p->uniforms[i].base.type,
            p->uniforms[i].binding,
            p->uniforms[i].base.buffer);
    }
}

static void start_compute(struct Pipeline* p1, int sx, int sy, int sz) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    int i;

    prog_use(p->programs[0]);

    assert(p->n_uniforms > 0);
    for (i = 0; i < p->n_uniforms; i++) {
        // TODO: uniforms? bad naming
        glBindBufferBase(
            p->uniforms[i].base.type,
            p->uniforms[i].binding,
            p->uniforms[i].base.buffer);
    }

    glDispatchCompute(sx, sy, sz);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // TODO
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
    glBindVertexArray(0);

    after(p);
}

static void draw_indexed(struct Pipeline* p1, int id, int index, int index_byte_size) {
    struct PipelineImpl* p = (struct PipelineImpl*)p1;
    before(p);
    if (index_byte_size < 2) {
        // TODO
        return;
    }
    glBindVertexArray(p->buffers[id].vao);

    struct BufferImpl* ibuf = p->b->get(p->b, index);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf->buffer);
    assert(index_byte_size == 2 || index_byte_size == 4 || index_byte_size == 1);

    int n_vertices = ibuf->size / index_byte_size;

    //printf("Draw: %d\n", n_vertices);
    GLenum type;
    if (index_byte_size == 2) {
        type = GL_UNSIGNED_SHORT;
    } else if (index_byte_size == 4) {
        type = GL_UNSIGNED_INT;
    } else {
        type = GL_UNSIGNED_BYTE;
    }
    if (p->geometry == GEOM_POINTS) {
        glDrawElements(GL_POINTS, n_vertices, type, NULL);
    } else {
        glDrawElements(GL_TRIANGLES, n_vertices, type, NULL);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

struct PipelineBuilder* enable_cull(struct PipelineBuilder* p1, enum CullType cull) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    switch (cull) {
    case CULL_FRONT:
        p->enable_cull = 1;
        p->cull_mode = GL_FRONT;
        break;
    case CULL_BACK:
        p->enable_cull = 1;
        p->cull_mode = GL_BACK;
        break;
    case CULL_BOTH:
        p->enable_cull = 1;
        p->cull_mode = GL_FRONT_AND_BACK;
        break;
    default:
        assert(0);
    }
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

static int gl_filter(enum FilterType filter_type) {
    switch (filter_type) {
    case FILTER_NEAREST: return GL_NEAREST;
    case FILTER_LINEAR: return GL_LINEAR;
    case FILTER_NEAREST_MIPMAP_NEAREST: return GL_NEAREST_MIPMAP_NEAREST;
    case FILTER_LINEAR_MIPMAP_NEAREST: return GL_LINEAR_MIPMAP_NEAREST;
    case FILTER_NEAREST_MIPMAP_LINEAR: return GL_NEAREST_MIPMAP_LINEAR;
    case FILTER_LINEAR_MIPMAP_LINEAR: return GL_LINEAR_MIPMAP_LINEAR;
    default: verify(0);
    }
    return -1;
}

static int gl_wrap(enum WrapType wrap_type) {
    switch (wrap_type) {
    case WRAP_REPEAT: return GL_REPEAT;
    case WRAP_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
    case WRAP_CLAMP_TO_EDGE: return GL_CLAMP_TO_EDGE;
    case WRAP_CLAMP_TO_BORDER: return GL_CLAMP_TO_BORDER;
    default: verify(0); break;
    }
    return -1;
}

static struct PipelineBuilder* sampler_mag_filter(struct PipelineBuilder* p1, enum FilterType filter_type) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_MAG_FILTER, gl_filter(filter_type));
    return p1;
}

static struct PipelineBuilder* sampler_min_filter(struct PipelineBuilder* p1, enum FilterType filter_type) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_MIN_FILTER, gl_filter(filter_type));
    return p1;
}

static struct PipelineBuilder* sampler_wrap_s(struct PipelineBuilder* p1, enum WrapType wrap_type) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_WRAP_S, gl_wrap(wrap_type));
    return p1;
}

static struct PipelineBuilder* sampler_wrap_t(struct PipelineBuilder* p1, enum WrapType wrap_type) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    glSamplerParameteri(p->cur_sampler->id, GL_TEXTURE_WRAP_T, gl_wrap(wrap_type));
    return p1;
}


static struct PipelineBuilder* end_sampler(struct PipelineBuilder* p1) {
    struct PipelineBuilderImpl* p = (struct PipelineBuilderImpl*)p1;
    p->cur_sampler = NULL;
    return p1;
}

static struct PipelineBuilder* set_bmgr(struct PipelineBuilder* p1, struct BufferManager* b)
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
        .storage_assign = uniform_assign,
        .uniform_assign = uniform_assign,
        .buffer_assign = buffer_assign,
        .storage_swap = storage_swap,
        .start = start,
        .start_compute = start_compute,
        .draw = draw,
        .draw_indexed = draw_indexed,
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

    pl->enable_cull = p->enable_cull;
    pl->cull_mode = p->cull_mode;

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

        .storage_add = storage_add,
        .uniform_add = uniform_add,

        .begin_buffer = begin_buffer,
        .buffer_attribute = buffer_attribute,
        .end_buffer = end_buffer,

        .begin_sampler = begin_sampler,
        .sampler_mag_filter = sampler_mag_filter,
        .sampler_min_filter = sampler_min_filter,
        .sampler_wrap_s = sampler_wrap_s,
        .sampler_wrap_t = sampler_wrap_t,
        .end_sampler = end_sampler,

        .enable_depth = enable_depth,
        .enable_blend = enable_blend,
        .enable_cull = enable_cull,

        .geometry = geometry,

        .build = build
    };
    p->base = base;
    p->r = r;
    return (struct PipelineBuilder*)p;
}
