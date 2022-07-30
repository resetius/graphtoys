#pragma once

#include <time.h>

struct Time {
    struct timespec value;
};

struct Duration {
    struct timespec value;
};

struct Time now();
struct Duration duration(struct Time* from, struct Time* to);
double to_seconds(struct Duration* duration);
