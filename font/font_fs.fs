#version 410

in vec2 TexCoord;

out vec4 fragColor;
uniform sampler2D Texture;

void main() {
    fragColor = texture(Texture, TexCoord);
}
