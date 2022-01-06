#version 410
#extension GL_ARB_shading_language_420pack : enable

#ifdef GL_ARB_shading_language_420pack
layout (set = 0, binding = 0)
#endif
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
