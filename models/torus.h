#pragma once

#include <render/render.h>

#include <lib/object.h>

struct Config;
struct EventProducer;
struct Object* CreateTorus(struct Render* r, struct Config* cfg, struct EventProducer*);
