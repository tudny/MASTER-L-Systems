#version 430

layout(location = 0) in vec4 position;
layout(location = 1) in int colorId;

layout(std430, binding = 0) readonly buffer Colors {
    vec4 colors[];
};

uniform mat4 pvm;

out Vertex {
    vec3 color;
    vec3 position;
} Out;

void main() {
    gl_Position = pvm * position;

    Out.color = colors[colorId].rgb;
    Out.position = position.xyz / position.w;
}
