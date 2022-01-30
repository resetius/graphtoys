#pragma once

#include <lib/linmath.h>

struct StlVertex {
    vec3 col;
    vec3 norm;
    vec3 pos;
};

struct StlVertex* stl_load(const char* fname, int* nvertices);
