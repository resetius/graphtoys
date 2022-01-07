#version 410
#extension GL_ARB_separate_shader_objects : enable
layout (location = 0) in vec3 frontColor;
layout (location = 4) in vec3 backColor;
layout (location = 0) out vec4 fragment;

void main()
{
    if (gl_FrontFacing) {
        //fragment = vec4(frontColor, 1.0);
        fragment = vec4(frontColor, 1.0);
    } else {
        fragment = vec4(backColor, 1.0);
    }
}
