#version 430

layout(location = 0) in vec4 position;

uniform mat4 pvm;

out Vertex {
    vec3 color;
    vec3 position;
} Out;

void main() {
    gl_Position = pvm * position;

    Out.color = vec3(0.0, 1.0, 0.0);
    Out.position = position.xyz / position.w;
}
