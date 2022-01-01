#version 330
uniform mat4 MVP;
uniform vec3 T;
layout (location = 0) in vec3 vPos;
out vec2 coord;
void main()
{
    gl_Position = MVP*vec4(vPos, 1.0);
    coord = T.z*vec2(vPos);
    coord = vec2(coord.x+T.x, coord.y+T.y);
    //coord = T.z*vec2(vPos.x+T.x,vPos.y+T.y);
}
