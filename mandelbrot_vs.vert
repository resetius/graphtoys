#version 410
#extension GL_ARB_separate_shader_objects : enable

uniform MatrixBlock {
    uniform mat4 MVP;
    uniform vec3 T;
};

layout (location = 1) in vec3 vPos;
layout (location = 0) out vec2 coord;

void main()
{
    gl_Position = MVP*vec4(vPos, 1.0);
    coord = T.z*vec2(vPos);
    coord = vec2(coord.x+T.x, coord.y+T.y);
    //coord = T.z*vec2(vPos.x+T.x,vPos.y+T.y);
}
