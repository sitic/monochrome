#version 330 core
out vec4 FragColor;
in vec2 Texcoord;
uniform sampler2D texture_r;
uniform sampler2D texture_b;
uniform sampler2D texture_g;
uniform float norm;

void main() {
    FragColor.r = texture(texture_r, Texcoord).r / norm;
    FragColor.b = texture(texture_b, Texcoord).r / norm;
    FragColor.g = texture(texture_g, Texcoord).r / norm;
    FragColor.a = 1.0;
}