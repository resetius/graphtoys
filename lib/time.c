#include <string.h>
#include "time.h"

struct Time now() {
    struct Time res;
    clock_gettime(CLOCK_MONOTONIC_RAW, &res.value);
    return res;
}

struct Duration duration(struct Time* from, struct Time* to)
{
    struct Duration res; memset(&res, 0, sizeof(res));
    long diff = from->value.tv_nsec - to->value.tv_nsec;
    res.value.tv_sec = to->value.tv_sec - from->value.tv_sec;
    if (diff < 0) {
        res.value.tv_sec -= 1;
        res.value.tv_nsec = 1000000000 + diff;
    } else {
        res.value.tv_nsec = diff;
    }
    return res;
}

double to_seconds(struct Duration* duration)
{
    double res = duration->value.tv_sec;
    res += duration->value.tv_nsec / 1000000000.0;
    return res;
}
