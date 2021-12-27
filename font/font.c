#include <stdlib.h>
#include <stdio.h>
#include <freetype/freetype.h>
#include "font.h"
#include "RobotoMono-Regular.h"

struct FontImpl {
    struct Font base;
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
    return (struct Font*)t;
}

void font_free(struct Font* f) {
    free(f);
}
