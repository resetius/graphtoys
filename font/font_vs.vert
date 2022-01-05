#version 410

uniform MatrixBlock {
    mat4 MVP;
};

layout (location = 1) in vec4 vPos;
layout (location = 0) out vec2 TexCoord;

void main()
{
    gl_Position = MVP*vec4(vPos.xy, 0.0, 1.0);
    TexCoord = vPos.zw;
}
