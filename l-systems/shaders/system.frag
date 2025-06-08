#version 430

in FVertex {
    vec3 color;
    vec3 position;
    vec3 normal;
} In;

uniform vec4 eyepos;
uniform vec3 ls_ambient;
uniform vec4 ls_position;
uniform vec3 ls_attenuation;
uniform vec3 ls_direct;

out vec4 out_color;

vec3 posDifference(vec4 p, vec3 pos, out float dist) {
    vec3 v = p.xyz;

    if (p.w != 0.0) {
        v = v / p.w - pos.xyz;
        dist = sqrt(dot(v, v));
    }
    return normalize(v);
}

float attFactor(vec3 att, float dist) {
    return 1.0 / (((att.z * dist) + att.y) * dist + att.x);
}

vec3 LambertLighting() {
    float dist;
    vec3 vv = posDifference(eyepos, In.position, dist);
    vec3 normal = normalize(In.normal);

    vec3 color = ls_ambient * In.color;
    vec3 lv = posDifference (ls_position, In.position, dist);
    float d = dot (lv, normal);
    if (dot (vv, normal) > 0.0) {
        if (d > 0.0) {
            if (ls_position.w != 0.0)
                d *= attFactor (ls_attenuation, dist);
            color += (d * ls_direct) * In.color;
        }
    }
    else {
        if (d < 0.0) {
            if (ls_position.w != 0.0)
                d *= attFactor (ls_attenuation, dist);
            color -= (d * ls_direct) * In.color;
        }
    }
    return clamp (color, 0.0, 1.0);
}

#define AGamma(colour) pow(colour, vec3(256.0 / 563.0))

void main() {
    //    out_color = vec4(In.color, 1.0);
    out_color = vec4(AGamma(LambertLighting()), 1.0);
}
