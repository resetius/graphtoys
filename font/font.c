#include <stdlib.h>
#include <stdio.h>
#include <freetype/freetype.h>
#include "font.h"
#include "font_vs.h"
#include "font_fs.h"
#include "mesh.h"
#include "program.h"
#include "RobotoMono-Regular.h"

struct FontImpl {
    struct Font base;
    struct Mesh* m;
    struct Program* p;
    FT_Library library;
    FT_Face face;
};

void font_render(struct Font* o, float x, float y, const char* string) {
    struct FontImpl* f = (struct FontImpl*)o;
    int error = FT_Load_Char(f->face, 'A', FT_LOAD_RENDER);
    if (error) {
        printf("Cannot load char: %d\n", error);
        exit(1);
    }
}

struct Font* font_new() {
    struct FontImpl* t = calloc(1, sizeof(*t));

    struct Vertex1 vertices[] = {
        {{-1.0f, 1.0f,0.0f}},
        {{-1.0f,-1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},

        {{-1.0f, 1.0f,0.0f}},
        {{ 1.0f,-1.0f,0.0f}},
        {{ 1.0f, 1.0f,0.0f}}
    };

    struct Font base = {
        .render = font_render
    };
    t->base = base;

    int error = FT_Init_FreeType(&t->library);
    if (error) {
        printf("Cannot load FreeType\n");
        exit(1);
    }
    error = FT_New_Memory_Face(
        t->library,
        (const FT_Byte*)font_RobotoMono_Regular,
        font_RobotoMono_Regular_size,
        0,
        &t->face);
    if (error) {
        printf("Cannot load Font\n");
        exit(1);
    }
    error = FT_Set_Char_Size(
          t->face,    /* handle to face object           */
          0,       /* char_width in 1/64th of points  */
          16*64,   /* char_height in 1/64th of points */
          300,     /* horizontal device resolution    */
          300 );   /* vertical device resolution      */
    if (error) {
        printf("Cannot set char size\n");
        exit(1);
    }

    t->m = mesh1_new(vertices, 6, 0);
    t->p = prog_new();

    prog_add_vs(t->p, font_font_vs);
    prog_add_fs(t->p, font_font_fs);
    prog_link(t->p);

    return (struct Font*)t;
}

void font_free(struct Font* o) {
    struct FontImpl* f = (struct FontImpl*)o;
    mesh_free(f->m);
    prog_free(f->p);
    free(f);
}
