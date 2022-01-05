#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include <freetype/freetype.h>

#include <render/program.h>
#include <render/pipeline.h>
#include <render/char.h>
#include <render/render.h>

#include <font/font_vs.vert.h>
#include <font/font_fs.frag.h>
#include <font/font_vs.vert.spv.h>
#include <font/font_fs.frag.spv.h>
#include <font/RobotoMono-Regular.ttf.h>

#include "font.h"
#include "linmath.h"

struct UniformBlock {
    mat4x4 mvp;
};

struct FontImpl {
    //struct Program* p;
    struct Char* chars[65536];
    struct UniformBlock uniform;
    struct Pipeline* pl;
    int current_id;
    int max_letters;
};

static void char_load(struct Render* r, struct Char* chars[], FT_Face face, wchar_t ch) {
    struct Char** out = &chars[ch];
    int error;
    error = FT_Load_Char(face, ch, FT_LOAD_RENDER);
    if (error) {
        printf("Cannot load char: %d\n", error);
        exit(1);
    }
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        printf("Cannot render glyph: %d\n", error);
        exit(1);
    }
    *out = rend_char_new(r, ch, face);
}

static int max(int a, int b) {
    return a>b?a:b;
}

static void char_dim(FT_Face face, wchar_t ch, int* w, int* h) {
    int error;
    error = FT_Load_Char(face, ch, FT_LOAD_RENDER);
    if (error) {
        printf("Cannot load char: %d\n", error);
        exit(1);
    }
    error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    if (error) {
        printf("Cannot render glyph: %d\n", error);
        exit(1);
    }
    FT_Bitmap bitmap = face->glyph->bitmap;
    *w = max(*w, bitmap.width);
    *h = max(*h, bitmap.rows);
}

struct Font* font_new(struct Render* r) {
    struct FontImpl* t = calloc(1, sizeof(*t));
    int i;
    FT_Library library;
    FT_Face face;
    const wchar_t* cyr =
        L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
        L"абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

    int error = FT_Init_FreeType(&library);
    if (error) {
        printf("Cannot load FreeType\n");
        exit(1);
    }
    error = FT_New_Memory_Face(
        library,
        (const FT_Byte*)font_RobotoMono_Regular_ttf,
        font_RobotoMono_Regular_ttf_size,
        0,
        &face);
    if (error) {
        printf("Cannot load Font\n");
        exit(1);
    }
    error = FT_Set_Char_Size(
          face,    /* handle to face object           */
          0,       /* char_width in 1/64th of points  */
          16*64,   /* char_height in 1/64th of points */
          300,     /* horizontal device resolution    */
          300 );   /* vertical device resolution      */
    if (error) {
        printf("Cannot set char size\n");
        exit(1);
    }

    t->max_letters = 100;

    memset(t->chars, 0, sizeof(t->chars));
    // load ascii
    for (i = 1; i < 128; i++) {
        char_load(r, t->chars, face, i);
    }
    while (*cyr) {
        char_load(r, t->chars, face, *cyr++);
    }

    struct ShaderCode vertex_shader = {
        .glsl = font_font_vs_vert,
        .spir_v = font_font_vs_vert_spv,
        .size = font_font_vs_vert_spv_size,
    };
    struct ShaderCode fragment_shader = {
        .glsl = font_font_fs_frag,
        .spir_v = font_font_fs_frag_spv,
        .size = font_font_fs_frag_spv_size,
    };

    struct PipelineBuilder* p = r->pipeline(r);
    struct Pipeline* pl = p->begin_program(p)
        ->add_vs(p, vertex_shader)
        ->add_fs(p, fragment_shader)
        ->end_program(p)

        ->begin_buffer(p, sizeof(vec4)) // {x,y,s,t}
        // set buffer data later in char render
        ->buffer_attribute(p, 1, 4, 4, 0)
        ->end_buffer(p)

        ->begin_uniform(p, 0, "MatrixBlock", sizeof(t->uniform))
        ->end_uniform(p)

        ->begin_sampler(p, 0)
        ->end_sampler(p)

        //->begin_descriptor(p)

        // for each texture

        //->end_descriptor(p)

        ->build(p);

    t->pl = pl;

    //pl->use_descriptor();
    //pl->use_buffer();
    //pl->submit();

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return (struct Font*)t;
}

void font_free(struct Font* o) {
    int i;
    struct FontImpl* f = (struct FontImpl*)o;
    if (f) {
        for (i = 0; i < sizeof(f->chars)/sizeof(struct Char*); i++) {
            if (f->chars[i]) {
                f->chars[i]->free(f->chars[i]);
            }
        }
        f->pl->free(f->pl);
        free(f);
    }
}

struct Label* label_new(struct Font* f) {
    struct FontImpl* f1 = (struct FontImpl*)f;
    struct Label* l = calloc(1, sizeof(*l));
    l->f = f;
    l->id = f1->current_id ++;
    return l;
}

void label_set_text(struct Label* l, const char* s) {
    int len = strlen(s);
    if (l->cap < len) {
        l->cap = len+1;
        l->text = realloc(l->text, l->cap);
    }
    strcpy(l->text, s);
}

void label_set_vtext(struct Label* l, const char* format, ...) {
    int size = 200;
    va_list args;
    if (l->cap < size) {
        l->cap = size;
        l->text = realloc(l->text, size);
    }
    va_start(args, format);
    vsnprintf(l->text, l->cap, format, args);
    va_end(args);
}

void label_set_pos(struct Label* l, int x, int y) {
    l->x = x;
    l->y = y;
}

void label_set_screen(struct Label* l, int w, int h) {
    l->w = w;
    l->h = h;
}

static int next_or_die(unsigned char** p, jmp_buf* buf) {
    if (((**p) >> 6) != 0x2) {
        longjmp(*buf, 1);
    }
    return (*((*p)++)) & 0x3f;
}

void label_render(struct Label* l)
{
    struct FontImpl* f = (struct FontImpl*)l->f;
    unsigned char* s = (unsigned char*)l->text;
    int x = l->x;
    int y = l->y;
    mat4x4 m, p, mvp;
    struct Char* ch;
    int symbol;
    int id = 0;
    mat4x4_identity(m);
    mat4x4_ortho(p, 0, l->w, 0, l->h, 1.f, -1.f);
    p[3][2] = 0;
    mat4x4_mul(mvp, p, m);

    //prog_use(f->p);
    //prog_set_mat4x4(f->p, "MVP", &mvp);

    f->pl->uniform_update(f->pl, 0, mvp, 0, sizeof(mvp));
    f->pl->start(f->pl);

    jmp_buf buf;
    int except = setjmp(buf);
    while (*s && !except) {
        symbol = 0;
        if ((*s & 0x80) == 0) {
            symbol = *s++;
        } else if (((*s) >> 5) == 0x6) {
            symbol  = ((*s++) & 0x1f) << 6;
            symbol |= next_or_die(&s, &buf);
        } else if (((*s) >> 4) == 0xe) {
            symbol  = ((*s++) & 0xf)  << 12;
            symbol |= next_or_die(&s, &buf) << 6;
            symbol |= next_or_die(&s, &buf);
        } else if (((*s) >> 3) == 0x1e) {
            symbol  = ((*s++) & 0x7)  << 18;
            symbol |= next_or_die(&s, &buf) << 12;
            symbol |= next_or_die(&s, &buf) << 6;
            symbol |= next_or_die(&s, &buf);
        } else {
            longjmp(buf, 1);
        }

        if (isspace(symbol)) {
            ch = f->chars['o'];
        } else {
            ch = f->chars[symbol];
            if (!ch) {
                continue;
            }

            float xpos, ypos, w, h;
            xpos = x + ch->left;
            ypos = y - (ch->h - ch->top);
            w = ch->w;
            h = ch->h;

            float vertices[6][4] = {
                { xpos, ypos + h,     0.0, 0.0 },
                { xpos, ypos,         0.0, 1.0 },
                { xpos + w, ypos,     1.0, 1.0 },

                { xpos, ypos + h,     0.0, 0.0 },
                { xpos + w, ypos,     1.0, 1.0 },
                { xpos + w, ypos + h, 1.0, 0.0 }
            };

            int cur_id = id++;
            cur_id += f->max_letters*l->id; // TODO: buffers manager
            f->pl->buffer_update(f->pl, cur_id, 0, vertices, 0, sizeof(vertices));
            f->pl->use_texture(f->pl, ch->texture(ch));
            f->pl->draw(f->pl, cur_id);
        }
        if (!ch) {
            continue;
        }
        x += ch->advance>>6;
    }
    if (except) {
        printf("Bad utf-8 sequence\n");
    }
}

void label_free(struct Label* l) {
    if (l) {
        free(l->text);
        free(l);
    }
}
