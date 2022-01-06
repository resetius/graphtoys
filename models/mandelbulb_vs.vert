#version 410

layout (std140) uniform MatrixBlock {
    uniform mat4 MVP;
    uniform mat4 Rot;
    uniform vec4 T;
    uniform int NextType;
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
