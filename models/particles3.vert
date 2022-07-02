#version 430
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding=0) uniform MatrixBlock {
    mat4 MVP;
};

layout(std140, binding=1) buffer PosBuffer {
    vec4 Position[];
};

layout(std140, binding=2) buffer NewPosBuffer {
    vec4 NewPosition[];
};

layout(std140, binding=3) buffer VelBuffer {
    vec4 Velocity[];
};

layout(std140, binding=4) buffer AccelBuffer {
    vec4 Accel[];
};

layout (location = 1) in int idx;
layout (location = 0) out vec4 color;

void main()
{
    vec4 vPos = Position[idx];
    float mass = vPos.w;
    gl_Position = MVP * vec4(vec3(vPos), 1);
    gl_PointSize = max(1, min(30, pow(vPos.w, 1./3.))); // mass
    if (mass > 0) {
        color = vec4(1.0, 0.0, 1.0, 1.0);
    } else {
        color = vec4(0);
    }
}
