#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vks {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;
using Quat = glm::quat;

struct Color : Vec4 {
    constexpr Color() = default;
    constexpr Color(float r, float g, float b, float a = 1.0f) : Vec4(r, g, b, a) {}
    static Color fromRGB(float r, float g, float b);
    static Color fromHex(uint32_t hex);
};

enum class BlendMode { Off, Alpha, Additive };
enum class CullMode  { Off, Front, Back };
enum class Topology  { TriangleList, TriangleStrip, LineList, PointList };

enum class TextureFormat { RGBA8, R16G16B16A16SFloat, BC1, BC3 };

struct ClearFlags {
    enum : int { Color = 1, Depth = 2, Stencil = 4 };
    using Type = int;
};

}
