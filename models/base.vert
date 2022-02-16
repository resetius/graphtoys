#version 410
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
#extension GL_KHR_vulkan_glsl : enable

#if defined(GL_KHR_vulkan_glsl) || defined(GLSLC)
layout (std140, set = 0, binding = 0)
#else
layout (std140)
#endif
uniform MatrixBlock {
    mat4 ModelViewMatrix;
    mat4 MVP;
    mat4 NormalMatrix;
    vec4 LP;
};

layout (location = 4) in vec2 in_tex;
layout (location = 3) in vec3 in_color;
layout (location = 2) in vec3 in_norm;
layout (location = 1) in vec3 in_pos;

layout (location = 8) out vec2 f_tex;
layout (location = 7) out vec3 f_color;
layout (location = 6) out vec3 f_norm;
layout (location = 5) out vec4 f_pos;

void main()
{
    f_tex = in_tex;
    f_color = in_color;
    f_norm = normalize(vec3(NormalMatrix*vec4(in_norm,1.0)));
    f_pos = ModelViewMatrix * vec4(in_pos, 1.0);

    gl_Position = MVP * vec4(in_pos, 1.0);
}
