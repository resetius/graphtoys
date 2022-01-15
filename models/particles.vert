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
    float mass = vPos.w;
    gl_Position = MVP * vec4(vec3(vPos), 1);
//    gl_Position = vPos;
    gl_PointSize = min(30, 2*vPos.w); // mass
    if (mass > 0) {
        color = vec3(1.0, 0.0, 1.0);
    } else {
        color = vec3(0);
    }
}
