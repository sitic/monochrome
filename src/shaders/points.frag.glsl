#version 330 core
out vec4 FragColor;
uniform vec4 color;

void main() { // https://rubendv.be/posts/fwidth/
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    float dist_squared = dot(circCoord, circCoord);
    float alpha = smoothstep(0.9, 1.1, dist_squared);
    FragColor = color;
    FragColor.a -= alpha;
}