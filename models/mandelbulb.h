#pragma once

#include <render/render.h>

#include <lib/object.h>

struct Config;
struct EventProducer;
struct Object* CreateMandelbulb(struct Render*, struct Config*, struct EventProducer*);
