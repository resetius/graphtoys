#version 330
uniform mat4 MVP;
layout (location = 0) in vec3 vPos;
out vec2 coord;
void main()
{
    gl_Position = MVP*vec4(vPos, 1.0);
    coord = 2*vPos.xy;
}
