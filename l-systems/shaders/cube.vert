#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 vpm;

out Vertex {
    vec3 color;
    vec3 position;
} Out;

void main() {
    vec4 pos = vec4(position, 1.0);

    gl_Position = vpm * pos;
    Out.color = color;
    Out.position = pos.xyz / pos.w;
}
