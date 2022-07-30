#include <stdlib.h>
#include <ktx.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

#include <render/render.h>
#include <render/char.h>
#include <render/pipeline.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <lib/verify.h>

struct Program* prog_opengl_new();
struct BufferManager* buf_mgr_opengl_new(struct Render* r);

struct CharImpl {
    struct Char base;
    GLuint tex_id;
};

struct RenderImpl {
    struct Render base;
    struct RenderConfig cfg;
    GLFWwindow* window;
    GLint major;
    GLint minor;

    // counters
    // TODO: move to separate file
    uint32_t timestamp;
    int queries_per_frame;
    int queries_delay;
    int query;
    struct Counter counters[32];
    int ncounters;
    int query2counter[32];
    GLuint queries[256];
};

static void GLAPIENTRY
message_callback( GLenum source,
                  GLenum type,
                  GLuint id,
                  GLenum severity,
                  GLsizei length,
                  const GLchar* message,
                  const void* userParam )
{
    fprintf( stderr, "GL CALLBACK: %s source = 0x%x, type = 0x%x, severity = 0x%x, id=%u, message = %s\n",
             ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
             source, type, severity, id, message );
}

static void* char_texture_(struct Char* ch) {
    return & (((struct CharImpl*)ch)->tex_id);
}

static void char_free_(struct Char* ch) {
    if (ch) {
        struct CharImpl* c = (struct CharImpl*)ch;
        glDeleteTextures(1, &c->tex_id);
        free(c);
    }
}

static struct Char* rend_char_new_(struct Render* r, wchar_t ch, void* bm) {
    struct CharImpl* c = calloc(1, sizeof(*c));
    FT_Face face = (FT_Face)bm;
    FT_Bitmap bitmap = face->glyph->bitmap;
    struct Char base = {
        .texture = char_texture_,
        .free = char_free_
    };
    c->base = base;
    c->base.ch = ch;
    c->base.w = bitmap.width;
    c->base.h = bitmap.rows;
    c->base.left = face->glyph->bitmap_left;
    c->base.top = face->glyph->bitmap_top;
    c->base.advance = face->glyph->advance.x;

    if (bitmap.width*bitmap.rows == 0) {
        free(c);
        return NULL;
    }

    glGenTextures(1, &c->tex_id);

    glBindTexture(GL_TEXTURE_2D, c->tex_id);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, bitmap.pitch);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RED,
        bitmap.width,
        bitmap.rows, 0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        bitmap.buffer);

    return (struct Char*)c;
};

static void rend_free_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    glDeleteQueries(256, r->queries);
    free(r);
}

static void set_view_entity_(struct Render* r, void* data) {
    GLFWwindow* window = data;
    ((struct RenderImpl*)r)->window = window;
    glfwMakeContextCurrent(window);
}

static void set_viewport(struct Render* r, int w, int h) {
    glViewport(0, 0, w, h);
}

static void draw_begin_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    r->timestamp = r->query*r->queries_per_frame;
    glQueryCounter(r->queries[r->timestamp++], GL_TIMESTAMP);
}

static void draw_end_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    int prev_query = (r->query+1) % r->queries_delay;

    glfwSwapBuffers(r->window);

    GLuint64 data[32] = {0};
    for (int i = 0; i < r->timestamp-r->query*r->queries_per_frame; i++) {
        glGetQueryObjectui64v(
            r->queries[prev_query*r->queries_per_frame+i],
            GL_QUERY_RESULT,
            &data[i]
            );
    }

    uint64_t s = data[0];
    // i = 0 -- technical counter
    for (int i = 1; i < r->timestamp-r->query*r->queries_per_frame&&i<32;i++) {
        int j = r->query2counter[i];
        r->counters[j].value += data[i] - s;
        r->counters[j].count ++;
        s = data[i];
    }

    r->query = (r->query+1)%r->queries_delay;
}

static void screenshot(struct Render* r, void** data, int* w, int* h) {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    *w = viewport[2];
    *h = viewport[3];
    *data = malloc(*w**h*4);
    glReadPixels(0, 0, viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, *data);

    int* M = *data;
    for (int i = 0; i < *h/2; i++) {
        for (int j = 0; j < *w; j++) {
            int t = M[i**w+j];
            M[i**w+j] = M[(*h-i-1)**w+j];
            M[(*h-i-1)**w+j] = t;
        }
    }
}

static void gl_info() {
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    printf("Renderer: %s\n", renderer);
    printf("OpenGL version supported %s\n", version);

    //GLint count = 0;
    //glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
    //printf("Binary formats: %d\n", count);
}

static void init_(struct Render* r1) {
    struct RenderImpl* r = (struct RenderImpl*)r1;
    gladLoadGL(glfwGetProcAddress);

//    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    gl_info();

    glGetIntegerv(GL_MAJOR_VERSION, &r->major);
    glGetIntegerv(GL_MINOR_VERSION, &r->minor);
    glClearColor(
        r->cfg.clear_color[0],
        r->cfg.clear_color[1],
        r->cfg.clear_color[2],
        r->cfg.clear_color[3]);

    if (r->major > 4 || (r->major == 4 && r->minor >= 3)) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(message_callback, 0);
    }

    glfwSwapInterval(r->cfg.vsync != 0); // vsync
    GLint n_extensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n_extensions);
    for (int i = 0; i < n_extensions; i++) {
        printf("Ext: '%s'\n", glGetStringi(GL_EXTENSIONS, i));
    }

#define prnLimit(var)  do {                                        \
        GLint i = 0;                                               \
        glGetIntegerv(var, &i);                                    \
        printf(#var ": %d\n", i);                                  \
    } while (0)

    glGenQueries(256, r->queries);
    r->timestamp = 0;
    r->queries_per_frame = 32;
    r->queries_delay = 3;
    r->query = 0;

#define prnLimit3(var) do {                     \
        for (int k = 0; k < 3; k++) {           \
            GLint i = 0;                        \
            glGetIntegeri_v(var, k, &i);        \
            printf(#var ": %d %d\n", k, i);     \
        }                                       \
} while (0)

    prnLimit(GL_MAX_COMPUTE_ATOMIC_COUNTERS);
    prnLimit(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
    prnLimit3(GL_MAX_COMPUTE_WORK_GROUP_COUNT);
    prnLimit3(GL_MAX_COMPUTE_WORK_GROUP_SIZE);
#undef prnLimit
#undef prnLimit3
}

ktxTexture* ktx_ASTC2RGB(ktxTexture* tex);

static struct Texture* tex_new(struct Render* r, void* data, enum TexType tex_type)
{
    // TODO: check tex_type
    ktxTexture* texture = data;
    KTX_error_code result = KTX_SUCCESS;
    GLenum glError = 0;
    GLuint* tex_id = calloc(1, sizeof(tex_id)); // TODO
    GLenum target = 0; // TODO

    result = ktxTexture_GLUpload(texture, tex_id, &target, &glError);
    if (result != KTX_SUCCESS) {
        texture = ktx_ASTC2RGB(texture);
        result = ktxTexture_GLUpload(texture, tex_id, &target, &glError);
        ktxTexture_Destroy(texture);
    }
    verify (result == KTX_SUCCESS);

    return (struct Texture*)tex_id;
}

static void print_compute_stats_(struct Render* r1)
{
    struct RenderImpl* r = (struct RenderImpl*)r1;
    double total = 0;
    for (int i = 0; i < 32; i++) {
        if (r->counters[i].count) {
            double value = r->counters[i].value
                *1e-6
                /r->counters[i].count;

            total += value;

            printf("%02d %s %.2fms\n", i, r->counters[i].name, value);

            r->counters[i].count = 0;
            r->counters[i].value = 0;
        }
    }
    if (total > 0) {
        printf("Total: %.2fms\n", total);
    }
}

static int counter_new(struct Render* r1, const char* name, enum CounterType type)
{
    struct RenderImpl* r = (struct RenderImpl*)r1;
    r->counters[r->ncounters].name = name;
    r->counters[r->ncounters].type = type;
    return r->ncounters++;
}

static void counter_submit(struct Render* r1, int id) {
    struct RenderImpl* r = (struct RenderImpl*)r1;

    r->query2counter[r->timestamp-r->query*r->queries_per_frame] = id;
    glQueryCounter(r->queries[r->timestamp++], GL_TIMESTAMP);
}

struct PipelineBuilder* pipeline_builder_opengl(struct Render*);


struct Render* rend_opengl_new(struct RenderConfig cfg)
{
    struct RenderImpl* r = calloc(1, sizeof(*r));
    struct Render base = {
        .free = rend_free_,
        .char_new = rend_char_new_,
        .set_view_entity = set_view_entity_,
        .draw_begin = draw_begin_,
        .draw_end = draw_end_,
        .init = init_,
        .pipeline = pipeline_builder_opengl,
        .set_viewport = set_viewport,
        .buffer_manager = buf_mgr_opengl_new,
        .tex_new = tex_new,
        .print_compute_stats = print_compute_stats_,
        .counter_new = counter_new,
        .counter_submit = counter_submit,
        .screenshot = screenshot,
    };
    r->base = base;
    r->cfg = cfg;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#endif
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    return (struct Render*)r;
}
