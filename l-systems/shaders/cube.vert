#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(std430, binding = 0) buffer Translations {
    mat4 translations[];
};

uniform mat4 pvm;

out Vertex {
    vec3 color;
    vec3 position;
} Out;

void main() {
    mat4 model = translations[gl_InstanceID];
    vec4 pos = model * vec4(position, 1.0);

    gl_Position = pvm * pos;
    Out.color = color;
    Out.position = pos.xyz / pos.w;
}
