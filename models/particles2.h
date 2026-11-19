#pragma once

#include <lib/object.h>

struct Render;
struct Config;
struct EventProducer;
struct Object* CreateParticles2(struct Render* r, struct Config* config, struct EventProducer*);
