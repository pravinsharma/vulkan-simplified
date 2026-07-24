#include "vks/mesh.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace vks {

struct Mesh::Impl {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

namespace {

void parseOBJ(std::istream& in, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices) {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vec3 p;
            iss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (prefix == "vn") {
            Vec3 n;
            iss >> n.x >> n.y >> n.z;
            normals.push_back(glm::normalize(n));
        } else if (prefix == "vt") {
            Vec2 uv;
            iss >> uv.x >> uv.y;
            uvs.push_back(uv);
        } else if (prefix == "f") {
            std::array<int, 3> idx[3];
            char slash;
            for (int i = 0; i < 3; ++i) {
                std::string vert;
                iss >> vert;
                int p = 0, t = 0, n = 0;
                std::istringstream viss(vert);
                viss >> p;
                if (viss >> slash) viss >> t;
                if (viss >> slash) viss >> n;
                idx[i][0] = p;
                idx[i][1] = t;
                idx[i][2] = n;
            }

            for (int i = 0; i < 3; ++i) {
                Vertex v{};
                v.position = idx[i][0] > 0 ? positions[static_cast<size_t>(idx[i][0]) - 1] : Vec3{};
                v.normal   = idx[i][2] > 0 ? normals[static_cast<size_t>(idx[i][2]) - 1]  : Vec3{};
                v.uv       = idx[i][1] > 0 ? uvs[static_cast<size_t>(idx[i][1]) - 1]     : Vec2{};
                outVertices.push_back(v);
            }
            size_t base = outVertices.size() - 3;
            outIndices.push_back(static_cast<uint32_t>(base));
            outIndices.push_back(static_cast<uint32_t>(base + 1));
            outIndices.push_back(static_cast<uint32_t>(base + 2));
        }
    }
}

}

Mesh::~Mesh() = default;

Mesh Mesh::fromVertices(std::span<const Vertex> vertices) {
    Mesh mesh;
    mesh.pimpl = std::make_unique<Impl>();
    mesh.pimpl->vertices.assign(vertices.begin(), vertices.end());
    return mesh;
}

Mesh Mesh::load(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open mesh file: " + path);
    }

    Mesh mesh;
    mesh.pimpl = std::make_unique<Impl>();
    parseOBJ(file, mesh.pimpl->vertices, mesh.pimpl->indices);
    return mesh;
}

uint32_t Mesh::vertexCount() const {
    return static_cast<uint32_t>(pimpl->vertices.size());
}

uint32_t Mesh::indexCount() const {
    return static_cast<uint32_t>(pimpl->indices.size());
}

bool Mesh::hasIndices() const {
    return !pimpl->indices.empty();
}

std::span<const Vertex> Mesh::vertices() const {
    return std::span<const Vertex>(pimpl->vertices.data(), pimpl->vertices.size());
}

std::span<const uint32_t> Mesh::indices() const {
    return std::span<const uint32_t>(pimpl->indices.data(), pimpl->indices.size());
}

}
