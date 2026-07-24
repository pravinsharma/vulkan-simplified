#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "vks/types.hpp"

namespace vks {

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

class Mesh {
public:
    static Mesh fromVertices(std::span<const Vertex> vertices);
    static Mesh load(const std::string& path);

    uint32_t vertexCount() const;
    uint32_t indexCount() const;
    bool hasIndices() const;

    std::span<const Vertex> vertices() const;
    std::span<const uint32_t> indices() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
