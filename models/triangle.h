#pragma once

#include <lib/object.h>

struct Render;
struct Config;
struct EventProducer;
struct Object* CreateTriangle(struct Render* r, struct Config*, struct EventProducer*);
