#version 430
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 0)
uniform MatrixBlock {
    mat4 MVP;
};
layout (location = 1) in vec4 vPos;
layout (location = 0) out vec3 color;

void main()
{
    gl_Position = MVP * vPos;
//    gl_Position = vPos;
    gl_PointSize = 5;
    color = vec3(1.0, 0.0, 1.0);
}
