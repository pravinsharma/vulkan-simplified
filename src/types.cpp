#include "vulkan_simplified/types.hpp"

#include <algorithm>
#include <cstdint>

namespace vks {

Color Color::fromRGB(float r, float g, float b) {
    return Color(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

Color Color::fromHex(uint32_t hex) {
    const float r = static_cast<float>((hex >> 16) & 0xFF) / 255.0f;
    const float g = static_cast<float>((hex >> 8) & 0xFF) / 255.0f;
    const float b = static_cast<float>(hex & 0xFF) / 255.0f;
    return Color(r, g, b, 1.0f);
}

}
