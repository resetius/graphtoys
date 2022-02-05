#pragma once

#include <render/render.h>

#include <lib/object.h>

struct Config;
struct Object* CreateMandelbrot(struct Render*, struct Config*);
