#pragma once

#include <lib/object.h>

struct Render;
struct Config;
struct EventProducer;
struct Object* CreateParticles(struct Render* r, struct Config* cfg, struct EventProducer*);
