#include <stdio.h>
#include <stdlib.h>

#include "stats.h"
#include "render_impl.h"

struct VkStats {
    struct RenderImpl* r;
    int has_timestamps;
    float timestamp_period;
};

struct VkStats* vk_stats_new(struct RenderImpl* r) {
    struct VkStats* v = calloc(1, sizeof(*v));
    v->r = r;
    v->has_timestamps = r->properties.limits.timestampComputeAndGraphics;
    v->timestamp_period = r->properties.limits.timestampPeriod;
    printf("has_timestamps: %d\n", v->has_timestamps);
    printf("timestamp_period: %f\n", v->timestamp_period);

    //uint32_t n_counters = 0;
    //vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(
    //    r->phy_dev, r->graphics_family, &n_counters, 0, 0);
    //printf("n_counters: %d\n", n_counters);
    return v;
}

void vk_stats_free(struct VkStats* v) {
    free(v);
}
