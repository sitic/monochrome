#version 330 core
out vec4 FragColor;
in vec2 Texcoord;
uniform sampler2D texture0;
uniform float norm;

void main() {
    FragColor = texture(texture0, Texcoord) / norm;
    FragColor.a = 1.0;
}