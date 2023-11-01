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
    float point_size_mult;
    float rho0;
    float max_rho;
    int enable_hsv;
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

// specific colors for points
layout(std430, binding=6) readonly buffer ColorBuffer {
    vec4 Col[];
};

// density
layout(std430, binding=7) readonly buffer DensityBuffer {
    float Rho[]; // Input, \Laplace psi = 4 pi rho
};

layout (location = 1) in int idx;
layout (location = 0) out vec4 color;

const float M_PI = 3.14159265359;

// https://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
vec4 hsv2rgb(vec3 hsv) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(hsv.xxx + K.xyz) * 6.0 - K.www);
    return vec4(hsv.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsv.y), 1);
}

#define poff(i,k,j) (((i+nn)%nn)*nn*nn+((k+nn)%nn)*nn+((j+nn)%nn))
void main()
{
    float mass = Position[idx].w;

    gl_Position = MVP * vec4(vec3(Position[idx]), 1);
    //gl_Position = MVP * vec4(vec2(Position[idx]), 0, 1);
    gl_PointSize = point_size_mult*clamp(pow(mass, 1./3.), 1, 30);
    if (Col[idx].w < 0) {
        color = out_color;
    } else {
        color = Col[idx];
    }

    // 1. new_pos <- cur vel + cur accel (compute)
    // 2. new_accel (E+F) <- new_pos
    // 3. new_vel <- new_accel + cur accel

//    vec4 r = Position[idx] + tau*Velocity[idx] + 0.5 * tau*tau*Accel[idx];
    vec4 A = vec4(0);

    /*if (dot(Position[idx].xyz,Position[idx].xyz) < 100/3.0*100/3.0) {
        return;
    }*/
    if (Velocity[idx].w < 0) {
        return;
    }

    vec4 x = Position[idx]-origin;
    ivec4 ii = clamp(ivec4(floor(x/h)), 0, nn-1);
    x = (x-ii*h)/h;

    for (int i = 0; i < 2; i++) {
        for (int k = 0; k < 2; k++) {
            for (int j = 0; j < 2; j ++) {
                A += E[poff(ii.z+i,ii.y+k,ii.x+j)] *
                    abs((1-i-x.z)*(1-k-x.y)*(1-j-x.x))/a/a/a;
            }
        }
    }

    if (enable_hsv == 1) {
        float rho = 0.0;
        for (int i = 0; i < 2; i++) {
            for (int k = 0; k < 2; k++) {
                for (int j = 0; j < 2; j ++) {
                    // for color
                    rho += (Rho[poff(ii.z+i,ii.y+k,ii.x+j)] + rho0)
                        * abs((1-i-x.z)*(1-k-x.y)*(1-j-x.x));
                }
            }
        }

        rho /= 4.0 * M_PI * rho0;
        rho = clamp(rho, 0, max_rho) / max_rho;
        color = hsv2rgb(vec3(clamp(rho, 0.15, 1), 1, 1));
    }

    A += F[idx]/a/a/a - 2*dota/a * Velocity[idx];

    Velocity[idx] += 0.5*tau*(vec4(vec3(A),0) + Accel[idx]);
    Accel[idx] = vec4(vec3(A), 0);

    vec4 r = Position[idx] + tau*Velocity[idx] + 0.5 * tau*tau*Accel[idx];
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
