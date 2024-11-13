#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec3 translation;

uniform mat4 pvm;

out Vertex {
    vec3 color;
    vec3 position;
} Out;

void main() {
    vec4 pos = vec4(position + translation, 1.0);

    gl_Position = pvm * pos;
    Out.color = color;
    Out.position = pos.xyz / pos.w;
}
