#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 fragColor;

uniform mat3 view_matrix;

void main() {
    gl_Position = vec4(view_matrix * position, 1.0);
    fragColor = color;
}
