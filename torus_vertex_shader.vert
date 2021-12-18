#version 330

uniform mat4 MVP;

in vec3 vCol;
in vec3 vPos;
out vec3 color;

void main()
{
    gl_Position = MVP * vec4(vPos, 1.0);
    color = vCol*min(1.0,max(0.2,-2*gl_Position.z));
}
