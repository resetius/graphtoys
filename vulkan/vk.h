#pragma once

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define DECL_FUNC(name) \
    extern PFN_##name name;

#define L0_FUNC(name) DECL_FUNC(name)
#define L1_FUNC(name) DECL_FUNC(name)
#define L2_FUNC(name) DECL_FUNC(name)

#include "symbols.h"

#undef DECL_FUNC
#undef L0_FUNC
#undef L1_FUNC
#undef L2_FUNC
