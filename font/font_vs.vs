#version 410
uniform mat4 MVP;

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

void main()
{
    gl_Position = MVP*vec4(vPos, 1.0);
    TexCoord = texCoord;
}
