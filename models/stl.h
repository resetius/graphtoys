#pragma once

#include <render/render.h>

#include <lib/object.h>
#include <lib/config.h>

struct EventProducer;
struct Object* CreateStl(struct Render* r, struct Config* cfg, struct EventProducer*);
