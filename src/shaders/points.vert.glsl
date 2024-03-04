#version 330 core
layout (location = 0) in vec2 position;

// I don't understand why, but glPointSize() doesn't seem to work on Apple-silicon macOS
// even though glEnable(GL_PROGRAM_POINT_SIZE) is called and code works on Linux & Windows.
// Setting the point size in the vertex shader seems to work, though.
uniform float pointsize;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    gl_PointSize = pointsize;
}