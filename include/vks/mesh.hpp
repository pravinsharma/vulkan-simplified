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
    static Mesh fromVertices(std::span<const Vertex> vertices, std::span<const uint32_t> indices);
    static Mesh load(const std::string& path);

    uint32_t vertexCount() const;
    uint32_t indexCount() const;
    bool hasIndices() const;

    std::span<const Vertex> vertices() const;
    std::span<const uint32_t> indices() const;

    Mesh() = default;
    ~Mesh() = default;
    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

private:
    struct Impl {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };
    std::unique_ptr<Impl> pimpl;
};

}
