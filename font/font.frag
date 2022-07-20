#version 410
#extension GL_ARB_shading_language_420pack : enable
#extension GL_KHR_vulkan_glsl : enable

#if defined(GL_KHR_vulkan_glsl) || defined(GLSLC)
layout (set = 0, binding = 0)
#endif
uniform MatrixBlock {
    mat4 MVP;
    vec4 color;
};

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 fragColor;

#if defined(GL_KHR_vulkan_glsl) || defined(GLSLC)
layout (set = 1, binding = 0)
#endif
uniform sampler2D Texture;

void main() {
    float x = texture(Texture, TexCoord).x;
    fragColor = color * x;
}
