#version 460 core

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;

void main() {
    vec3 hdr = texture(inputTexture, inUV).rgb;
    vec3 bloom = texture(bloomTexture, inUV).rgb;
    outColor = vec4(hdr + bloom, 1.0);
}
