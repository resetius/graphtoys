#pragma once

struct VkStats;
struct RenderImpl;
struct VkStats* vk_stats_new(struct RenderImpl*);
void vk_stats_free(struct VkStats* );
