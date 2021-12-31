#pragma once

struct Render;
struct Program;
struct Texture;
struct Char;

struct Render* rend_opengl_new();
void rend_free(struct Render*);

struct Program* rend_prog_new(struct Render*);
struct Mesh* rend_mesh_new(struct Render*);
struct Texture* rend_tex_new(struct Render*);
struct Texture* rend_char_new(struct Render*);
