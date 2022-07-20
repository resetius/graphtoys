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

layout (location = 1) in vec4 vPos;
layout (location = 0) out vec2 TexCoord;

void main()
{
    gl_Position = MVP*vec4(vPos.xy, 0.0, 1.0);
    TexCoord = vPos.zw;
}
