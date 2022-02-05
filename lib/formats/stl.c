#include <stdio.h>
#include <stdlib.h>
#include <lib/verify.h>
#include "stl.h"

struct StlVertex* stl_load(const char* fname, int* nvertices) {
    FILE* f = fopen(fname, "rb");
    char header[100];
    int n_triangles;
    int i,j,k;
    struct StlVertex* res;
    float min_x, max_x;
    float min_y, max_y;
    float min_z, max_z;
    if (!f) {
        *nvertices = 0;
        return NULL;
    }

    min_x = min_y = min_z =  1e10;
    max_x = max_y = max_z = -1e10;

    verify(fread(header, 1, 80, f) == 80);
    header[80] = 0;
    printf("header: '%s'\n", header);
    verify(fread(&n_triangles, 4, 1, f) == 1);

    *nvertices = n_triangles*3;
    printf("ntriangles: %d\n", n_triangles);

    res = calloc(*nvertices, sizeof(struct StlVertex));

    k = 0;
    for (i = 0; i < n_triangles; i++) {
        float nx, ny, nz;
        int attrs = 0;
        verify(fread(&nx, 4, 1, f) == 1);
        verify(fread(&ny, 4, 1, f) == 1);
        verify(fread(&nz, 4, 1, f) == 1);

        for (j = 0; j < 3; j++) {
            float x, y, z;
            verify(fread(&x, 4, 1, f) == 1);
            verify(fread(&y, 4, 1, f) == 1);
            verify(fread(&z, 4, 1, f) == 1);

            struct StlVertex v = {
                {1.0, 1.0, 0.0},
                {nx, ny, nz},
                {x, y, z},
            };
            res[k++] = v;

            min_x = fmin(min_x, x);
            min_y = fmin(min_y, y);
            min_z = fmin(min_z, z);
            max_x = fmax(max_x, x);
            max_y = fmax(max_y, y);
            max_z = fmax(max_z, z);
        }
        verify(fread(&attrs, 2, 1, f) == 1);
        //int r = attrs & 0x1f;
        //int g = (attrs>>5) & 0x1f;
        //int b = (attrs>>10) & 0x1f;
        //float scale = 1./((1<<5)-1);
        //vec3 col = {(float)r*scale, (float)g*scale, (float)b*scale};
        //printf("%f %f %f\n", col[0], col[1], col[2]);
        //memcpy(res[k-1].col, col, sizeof(col));
        attrs = 0;
        fseek(f, attrs, SEEK_CUR);
    }

    float scale = 8./(fmax(max_x,fmax(max_y,max_z))-fmin(min_x,fmin(min_y,min_z)));
    for (i = 0; i < k; i++) {
        res[i].pos[0] -= 0.5*(max_x+min_x);
        res[i].pos[1] -= 0.5*(max_y+min_y);
        res[i].pos[2] -= 0.5*(max_z+min_z);
        vec3_scale(res[i].pos, res[i].pos, scale);
    }

    fclose(f);
    *nvertices = k;
    return res;
}
