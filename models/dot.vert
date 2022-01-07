#version 410
#extension GL_ARB_separate_shader_objects : enable
uniform MatrixBlock {
    mat4 MVP;
};
layout (location = 2) in vec3 vCol;
layout (location = 1) in vec3 vPos;
layout (location = 0) out vec3 color;
layout (location = 3) out vec3 origCoord;

void main()
{
    gl_Position = MVP * vec4(vPos, 1.0);
    origCoord = vPos;
    color = vCol;
}
