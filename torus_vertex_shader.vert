#version 410
#extension GL_ARB_separate_shader_objects : enable

layout (std140) uniform MatrixBlock {
    mat4 ModelViewMatrix;
    mat4 MVP;
    mat4 NormalMatrix;
};

//uniform mat4 ProjectionMatrix;

layout (location = 3) in vec3 vCol;
layout (location = 2) in vec3 vNorm;
layout (location = 1) in vec3 vPos;
layout (location = 0) out vec3 color;

void main()
{
    vec3 Ld = vec3(1.0, 1.0, 1.0);
    vec3 Kd = vec3(0.9, 0.5, 0.3);
    vec3 tnorm = normalize(vec3(NormalMatrix*vec4(vNorm,1.0)));
    vec4 eyeCoords = ModelViewMatrix * vec4(vPos, 1.0);

    vec4 LightPosition = vec4(5.0, 5.0, 2.0, 1.0);
    vec3 s = normalize(vec3(LightPosition - eyeCoords));

    color = Ld*Kd*max(dot(s, tnorm), 0.0);

    //vec4 LightPosition1 = vec4(15.0, -5.0, -2.0, -1.0);
    //vec3 s1 = normalize(vec3(LightPosition1 - eyeCoords));
    //color = color + Ld*Kd*max(dot(s1, tnorm), 0.0);
    //color = normalize(color);

    gl_Position = MVP * vec4(vPos, 1.0);
    //color = vCol*min(1.0,max(0.2,-2*gl_Position.z));
}
