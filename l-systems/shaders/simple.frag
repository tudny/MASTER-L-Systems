#version 420

layout(location = 0) in vec3 fragColor;

out vec4 color;

void main() {
    color = vec4(fragColor, 1.0);
}
