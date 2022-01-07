#include <stdio.h>

int main(int argc, char** argv) {
    FILE* f;
    char header[100];
    int n_triangles;
    int i,j;
    if (argc < 2) {
        printf("File name required\n");
        return -1;
    }
    f = fopen(argv[1], "rb");
    if (!f) {
        printf("Cannot open file '%s'\n", argv[1]);
        return -1;
    }
    fread(header, 1, 80, f);
    header[80] = 0;
    printf("header: '%s'\n", header);
    fread(&n_triangles, 4, 1, f);
    printf("ntriangles: %d\n", n_triangles);
    for (i = 0; i < n_triangles; i++) {
        float nx, ny, nz;
        int attrs = 0;
        fread(&nx, 4, 1, f);
        fread(&ny, 4, 1, f);
        fread(&nz, 4, 1, f);
        printf("{\n");
        printf("  normal: %f %f %f\n", nx, ny, nz);
        for (j = 0; j < 3; j++) {
            float x, y, z;
            fread(&x, 4, 1, f);
            fread(&y, 4, 1, f);
            fread(&z, 4, 1, f);
            printf("  v[%d]: %f %f %f\n", j, x, y, z);
        }
        fread(&attrs, 2, 1, f);
        int r = attrs & 0x1f;
        int g = (attrs>>5) & 0x1f;
        int b = (attrs>>10) & 0x1f;
        printf("  %d, %d, %d, %d\n", attrs, r, g, b);
        attrs = 0;
        printf("  attrs size: %d\n", attrs);
        fseek(f, attrs, SEEK_CUR);
        printf("}\n");
    }
    fclose(f);
    return 0;
}
