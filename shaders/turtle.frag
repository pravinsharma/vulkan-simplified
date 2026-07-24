#version 460 core

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTintColor;
layout(location = 4) in float inRoughnessOverride;
layout(location = 5) in float inMetallicOverride;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
    vec3 viewPosition;
};

void main() {
    vec3 n = normalize(inNormal);
    vec3 light = normalize(vec3(0.5, -1.0, 0.3));
    float diff = max(dot(n, -light), 0.0);
    vec3 base = inTintColor.rgb * (diff + 0.3);
    outColor = vec4(base, 1.0);
}
