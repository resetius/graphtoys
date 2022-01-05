#version 410

uniform MatrixBlock {
    mat4 MVP;
};

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 fragColor;

uniform sampler2D Texture;

void main() {
    float x = texture(Texture, TexCoord).x;
    fragColor = vec4(0.0, x, x, x);
}
