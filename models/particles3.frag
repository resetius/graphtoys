#version 430
#extension GL_ARB_separate_shader_objects : enable

layout(std140, binding=0) uniform MatrixBlock {
    mat4 MVP;
    vec4 origin;
    vec4 out_color;
    vec4 viewport;
    float ratio;
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

layout (location = 0) in vec4 color;
layout (location = 1) in vec2 vertexPos;

layout (location = 0) out vec4 fragment;

void main()
{
    vec2 aspect = vec2(1, 1.0 / ratio);

    // https://stackoverflow.com/questions/38938498/how-do-i-convert-gl-fragcoord-to-a-world-space-point-in-a-fragment-shader
    
    vec2 fragCoord = (((2*gl_FragCoord.xy) - (2*viewport.xy)) / (viewport.zw) - 1) * aspect;
    vec2 vertex = vertexPos * aspect;

    float distanceFromCenter = length(fragCoord - vertex);
    if (distanceFromCenter <= 1 / (gl_FragCoord.z / gl_FragCoord.w)) {
        fragment = color;
    } else {
        fragment = vec4(0, 0, 0, 0);
    }
}
