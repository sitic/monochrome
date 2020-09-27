#version 330 core
layout (points) in;
layout (line_strip, max_vertices = 5) out;
uniform vec2 halfwidth;

in VS_OUT {
    vec3 color;
} gs_in[];

out vec3 fColor;

void build_rect(vec4 position) {
    fColor = gs_in[0].color;// gs_in[0] since there's only one input vertex
    vec4 vert = position + vec4(-halfwidth.x, -halfwidth.y, 0.0, 0.0);// bottom-left
    gl_Position = clamp(vert, -1., 1.);
    EmitVertex();
    vert = position + vec4(halfwidth.x, -halfwidth.y, 0.0, 0.0);// bottom-right
    gl_Position = clamp(vert, -1., 1.);
    EmitVertex();
    vert = position + vec4(halfwidth.x, halfwidth.y, 0.0, 0.0);// top-right
    gl_Position = clamp(vert, -1., 1.);
    EmitVertex();
    vert = position + vec4(-halfwidth.x, halfwidth.y, 0.0, 0.0);// top-left
    gl_Position = clamp(vert, -1., 1.);
    EmitVertex();
    vert = position + vec4(-halfwidth.x, -halfwidth.y, 0.0, 0.0);// bottom-left
    gl_Position = clamp(vert, -1., 1.);
    EmitVertex();
    EndPrimitive();
}

void main() {
    build_rect(gl_in[0].gl_Position);
}