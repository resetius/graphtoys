#pragma once

struct Render;
struct Program;

struct Render* rend_new();
void rend_free(struct Render*);

struct Program* rend_prog_new(struct Render*);
struct Mesh* rend_mesh_new(struct Render*);
