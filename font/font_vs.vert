#version 410
uniform mat4 MVP;

layout (location = 0) in vec4 vPos;
out vec2 TexCoord;

void main()
{
    gl_Position = MVP*vec4(vPos.xy, 0.0, 1.0);
    TexCoord = vPos.zw;
}
