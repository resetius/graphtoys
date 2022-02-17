#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <assert.h>

#define DECL_FUNC(name)                         \
    PFN_##name name;                            \
    PFN_##name ktx_##name;


#define L0_FUNC(name) DECL_FUNC(name)
#define L1_FUNC(name) DECL_FUNC(name)
#define L2_FUNC(name) DECL_FUNC(name)

#include "symbols.h"

#undef DECL_FUNC
#undef L0_FUNC
#undef L1_FUNC
#undef L2_FUNC

void vk_load_global() {
#define L0_FUNC(name) \
    ktx_##name = name = (PFN_##name)glfwGetInstanceProcAddress(NULL, #name); \
    assert(name);

#include "symbols.h"

#undef L0_FUNC
}

void vk_load_instance(VkInstance instance) {
#define L1_FUNC(name) \
    ktx_##name = name = (PFN_##name)glfwGetInstanceProcAddress(instance, #name); \
    assert(name);

#include "symbols.h"

#undef L1_FUNC
}

void vk_load_device(VkDevice device) {
#define L2_FUNC(name) \
    ktx_##name = name = (PFN_##name)vkGetDeviceProcAddr(device, #name); \
    assert(name)

#include "symbols.h"

#undef L2_FUNC
}

#include <ktxvulkan.h>

void* ktxVulkanModuleHandle = (void*)1; // hack
ktx_error_code_e ktxLoadVulkanLibrary(void) { return 0; }
