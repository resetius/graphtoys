#pragma once

#include <lib/object.h>
#include <lib/event.h>

struct Render;
struct Config;
struct Object* CreateParticles3(struct Render* r, struct Config* cfg, struct EventProducer* events);
