#version 410

layout (location = 0) in vec3 color;
layout (location = 0) out vec4 fragment;

void main()
{
    fragment = vec4(color, 1.0);
}
