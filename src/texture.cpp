#include "vks/texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace vks {

namespace {

bool isPNG(std::span<const uint8_t> data) {
    return data.size() >= 4 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G';
}

}

struct Texture::Impl {
    uint32_t width = 0;
    uint32_t height = 0;
    TextureFormat format = TextureFormat::RGBA8;
    bool hasMips = false;
    std::vector<uint8_t> pixels;
};

Texture Texture::load(const std::string& path, const TextureDesc& desc) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open texture file: " + path);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read texture file: " + path);
    }

    int w = 0, h = 0, comp = 0;
    if (!isPNG(buffer)) {
        throw std::runtime_error("Only PNG textures are supported: " + path);
    }

    unsigned char* pixels = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &w, &h, &comp, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("stbi_load failed for: " + path + " - " + stbi_failure_reason());
    }

    Texture tex;
    tex.pimpl = std::make_unique<Impl>();
    tex.pimpl->width = static_cast<uint32_t>(w);
    tex.pimpl->height = static_cast<uint32_t>(h);
    tex.pimpl->format = TextureFormat::RGBA8;
    tex.pimpl->hasMips = desc.generateMips && (w > 1 || h > 1);

    size_t pixelCount = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
    tex.pimpl->pixels.resize(pixelCount);
    std::memcpy(tex.pimpl->pixels.data(), pixels, pixelCount);

    stbi_image_free(pixels);
    return tex;
}

uint32_t Texture::width() const {
    return pimpl->width;
}

uint32_t Texture::height() const {
    return pimpl->height;
}

TextureFormat Texture::format() const {
    return pimpl->format;
}

bool Texture::hasMips() const {
    return pimpl->hasMips;
}

}
