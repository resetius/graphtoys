#version 430
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding=0) uniform MatrixBlock {
    mat4 MVP;
    int total;
};

layout(std140, binding=1) buffer Pos {
    vec4 Position[];
};

layout(std140, binding=2) buffer Vel {
    vec4 Velocity[];
};

layout(std140, binding=3) buffer Tmp {
    vec4 Accel[];
};

layout(std140, binding=4) buffer NewPos {
    vec4 NewPosition[];
};

layout (location = 1) in int idx;
layout (location = 0) out vec3 color;

void main()
{
    vec4 vPos = Position[idx];
    float mass = vPos.w;
    gl_Position = MVP * vec4(vec3(vPos), 1);
//    gl_Position = vPos;
    gl_PointSize = max(1, min(30, 2*vPos.w)); // mass
    if (mass > 0) {
        color = vec3(1.0, 0.0, 1.0);
    } else {
        color = vec3(0);
    }

    float DeltaT = 0.001;

    float newMass = Position[idx].w;
    bool erase = false;
    float G = 1;
    vec4 r = vec4(0);
    vec3 A = vec3(0);
    // Verlet integration
    if (length(vec3(Position[idx])) < 1e6 && Position[idx].w > 0) {
        // (1) R(t2)
        r = Position[idx] + DeltaT*Velocity[idx] + 0.5 * DeltaT * DeltaT * Accel[idx];

        // (3) new Accel
        for (int j = 0; j < total && !erase; j++) {
            if (idx != j && Position[j].w > 0) {
                float R = length(vec3(r-Position[j]));
                float r1 = min(30, (Position[idx].w+Position[j].w))*0.0004;
                if (R > r1) {
                    float mass = Position[j].w;
                    vec3 otherA = G * mass * vec3(Position[j]-r) / R / R / R;
                    A = A + otherA;
                } else {
                    if (j < idx) {
                        // elliminate
                        erase = true;
                    } else {
                        // add mass
                        newMass += Position[j].w;
                    }
                }
            }
        }

    } else {
        erase = true;
    }

    if (erase) {
        newMass = 0;
    }

    // (4) new Velocity
    Velocity[idx] = Velocity[idx] + 0.5 * DeltaT * (vec4(A, 0) + Accel[idx]);
    Accel[idx] = vec4(A, 0);
    NewPosition[idx] = vec4(vec3(r), newMass);
}
