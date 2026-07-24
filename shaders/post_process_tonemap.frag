#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;

void main() {
    vec3 hdr = texture(inputTexture, inUV).rgb;
    float exposure = 1.0;
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);
    outColor = vec4(mapped, 1.0);
}
