#version 430

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in Vertex {
    vec3 color;
    vec3 position;
} In[];

out FVertex {
    vec3 color;
    vec3 position;
    vec3 normal;
} Out;

void main(void) {
    vec3 a = In[0].position;
    vec3 b = In[1].position;
    vec3 c = In[2].position;

    vec3 normal = normalize(cross(b - a, c - a));

    for (int i = 0; i < 3; i++) {
        Out.color = In[i].color;
        Out.position = In[i].position;
        Out.normal = normal;
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }

    EndPrimitive();
}
