#version 430
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding=0) uniform MatrixBlock {
    mat4 MVP;
    vec4 origin;
    vec4 out_color;
    float h;
    float DeltaT;
    float a;
    float dota;
    int nn;
};

float l = h*nn;

layout(std140, binding=1) buffer PosBuffer {
    vec4 Position[];
};

layout(std140, binding=2) buffer VelBuffer {
    vec4 Velocity[];
};

layout(std140, binding=3) buffer AccelBuffer {
    vec4 Accel[];
};

layout(std430, binding=4) buffer EBuffer {
    vec4 E[];
};

// pp-force
layout(std430, binding=5) buffer ForceBuffer {
    vec4 F[];
};

layout (location = 1) in int idx;
layout (location = 0) out vec4 color;

void distribute(inout float M[2][2][2], in vec4 x, out ivec4 idx, float h) {
    idx = ivec4(
        floor(x.x / h),
        floor(x.y / h),
        floor(x.z / h),
        0
        );

    x = (x-idx*h)/h;

    M[0][0][0] = (1-x.z)*(1-x.y)*(1-x.x);
    M[0][0][1] = (1-x.z)*(1-x.y)*(x.x);
    M[0][1][0] = (1-x.z)*(x.y)*(1-x.x);
    M[0][1][1] = (1-x.z)*(x.y)*(x.x);

    M[1][0][0] = (x.z)*(1-x.y)*(1-x.x);
    M[1][0][1] = (x.z)*(1-x.y)*(x.x);
    M[1][1][0] = (x.z)*(x.y)*(1-x.x);
    M[1][1][1] = (x.z)*(x.y)*(x.x);
}

#define poff(i,k,j) (((i+nn)%nn)*nn*nn+((k+nn)%nn)*nn+((j+nn)%nn))
void main()
{
    vec4 vPos = Position[idx];
    float mass = vPos.w;

    gl_Position = MVP * vec4(vec3(vPos), 1);
    gl_PointSize = max(1, min(30, pow(vPos.w, 1./3.))); // mass
    if (mass > 0) {
        color = out_color;
    } else {
        color = vec4(0);
    }

    float M[2][2][2];
    vec4 r = Position[idx] + DeltaT*Velocity[idx] /*/a*/ + 0.5 * DeltaT*DeltaT*Accel[idx];
    vec4 A = vec4(0);
    ivec4 ii;
    distribute(M, vPos-origin, ii, h);

    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 2; k++) {
            for (int j = 0; j < 2; j ++) {
                for (int m = 0; m < 3; m++) {
                    A[m] += E[poff(ii.z+i,ii.y+k,ii.x+j)][m] * M[i][k][j]/a/a/a;
                }
            }
        }
    }

    A += F[idx]/a/a;
    //A -= 2*DeltaT*dota/a * Velocity[idx];
    Velocity[idx] = Velocity[idx]
        - 2*DeltaT*dota/a * Velocity[idx]
        + 0.5 * DeltaT * (vec4(vec3(A),0) + Accel[idx]);
    Accel[idx] = vec4(vec3(A), 0);
    //NewPosition[idx] = vec4(vec3(r), mass);
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
