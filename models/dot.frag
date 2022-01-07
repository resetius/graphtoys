#version 410
#extension GL_ARB_separate_shader_objects : enable

layout (location = 3) in vec3 origCoord;
layout (location = 0) in vec3 color;
layout (location = 0) out vec4 fragment;

void main()
{
    if (length(origCoord) < 0.1) {
        fragment = vec4(1.0, 1.0, 1.0, 1.0);
    } else {
        fragment = vec4(0.0);
    }
    //fragment = vec4(vec3(ptc), 1.0);
    //fragment = vec4(color, 1.0);
    //fragment = vec4(1.0, 0.0, 0.0, 1.0);
}
