#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) uniform Push {
    mat4 model;
    vec4 tintColor;
    float roughnessOverride;
    float metallicOverride;
} push;

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
    vec3 viewPosition;
};

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outTintColor;
layout(location = 4) out float outRoughnessOverride;
layout(location = 5) out float outMetallicOverride;

void main() {
    vec4 world = push.model * vec4(inPosition, 1.0);
    outWorldPos = world.xyz;
    outNormal = mat3(push.model) * inNormal;
    outUV = inUV;
    outTintColor = push.tintColor;
    outRoughnessOverride = push.roughnessOverride;
    outMetallicOverride = push.metallicOverride;
    gl_Position = viewProjection * world;
}
