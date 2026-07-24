#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "vks/types.hpp"

namespace vks {

struct TextureDesc {
    TextureFormat format = TextureFormat::RGBA8;
    bool generateMips = true;
};

class Texture {
public:
    static Texture load(const std::string& path, const TextureDesc& desc = {});
    uint32_t width() const;
    uint32_t height() const;
    TextureFormat format() const;
    bool hasMips() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
