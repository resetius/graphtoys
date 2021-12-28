#version 410

in vec2 TexCoord;

out vec4 fragColor;
uniform sampler2D Texture;

void main() {
    float x = texture(Texture, TexCoord).x;
    fragColor = vec4(0.0, x, x, x);
}
