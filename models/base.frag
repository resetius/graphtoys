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

#if defined(GL_KHR_vulkan_glsl) || defined(GLSLC)
layout (set = 1, binding = 0)
#endif
uniform sampler2D Texture;

layout (location = 8) in vec2 f_tex;
layout (location = 7) in vec3 f_color;
layout (location = 6) in vec3 f_norm;
layout (location = 5) in vec4 f_pos;

layout (location = 0) out vec4 fragment;

void main()
{
    vec3 Ld = vec3(1.0, 1.0, 1.0);
    vec3 Kd = f_color;

    if (Kd.x == 0 && Kd.y == 0 && Kd.z == 0) {
        // TODO
        Kd = texture(Texture, f_tex).xyz; // f_color;
    }
    vec3 tnorm = f_norm;
    vec4 eyeCoords = f_pos;

    vec4 LightPosition = LP;

    vec3 s = vec3(0);
    if (LightPosition.w == 0.0) {
        s = normalize(vec3(LightPosition));
    } else {
        s = normalize(vec3(LightPosition - eyeCoords));
    }

    vec3 frontColor = Ld*Kd*0.15+Ld*Kd*max(dot(s, tnorm), 0.0);
    vec3 backColor = Ld*Kd*0.15+Ld*Kd*max(dot(s, -tnorm), 0.0);

    if (gl_FrontFacing) {
        fragment = vec4(frontColor, 1.0);
    } else {
        fragment = vec4(backColor, 1.0);
    }
}
