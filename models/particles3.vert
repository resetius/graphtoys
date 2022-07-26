#version 430
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding=0) uniform MatrixBlock {
    mat4 MVP;
    vec4 origin;
    vec4 out_color;
    float h;
    float tau;
    float a;
    float dota;
    int nn;
};

float l = h*nn;

layout(std430, binding=1) buffer PosBuffer {
    vec4 Position[];
};

layout(std430, binding=2) buffer VelBuffer {
    vec4 Velocity[];
};

layout(std430, binding=3) buffer AccelBuffer {
    vec4 Accel[];
};

// field strength
layout(std430, binding=4) readonly buffer EBuffer {
    vec4 E[];
};

// pp-force
layout(std430, binding=5) readonly buffer ForceBuffer {
    vec4 F[];
};

layout (location = 1) in int idx;
layout (location = 0) out vec4 color;

#define poff(i,k,j) (((i+nn)%nn)*nn*nn+((k+nn)%nn)*nn+((j+nn)%nn))
void main()
{
    float mass = Position[idx].w;

    gl_Position = MVP * vec4(vec3(Position[idx]), 1);
    gl_PointSize = clamp(pow(mass, 1./3.), 1, 30);
    color = out_color;

    vec4 r = Position[idx] + tau*Velocity[idx] + 0.5 * tau*tau*Accel[idx];
    vec4 A = vec4(0);

    vec4 x = Position[idx]-origin;
    ivec4 ii = ivec4(floor(x/h));
    x = (x-ii*h)/h;

    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 2; k++) {
            for (int j = 0; j < 2; j ++) {
                A += E[poff(ii.z+i,ii.y+k,ii.x+j)] *
                    abs((1-i-x.z)*(1-k-x.y)*(1-j-x.x))/a/a/a;
            }
        }
    }

    A += F[idx]/a/a - 2*tau*dota/a * Velocity[idx];
    Velocity[idx] += 0.5*tau*(vec4(vec3(A),0) + Accel[idx]);
    Accel[idx] = vec4(vec3(A), 0);
    for (int i = 0; i < 3; i++) {
        if (r[i] < origin[i]) {
            r[i] += l;
        }
        if (r[i] >= origin[i]+l) {
            r[i] -= l;
        }
    }
    Position[idx] = vec4(vec3(r), mass);
}
