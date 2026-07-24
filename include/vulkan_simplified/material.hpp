#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "vulkan_simplified/types.hpp"

namespace vks {

class Texture;

class Material;

struct PBRTextures {
    const Texture* albedo = nullptr;
    const Texture* normal = nullptr;
    const Texture* metallicRoughness = nullptr;
    const Texture* ao = nullptr;
    const Texture* emissive = nullptr;
};

struct PBRUniforms {
    float metallic = 0.0f;
    float roughness = 0.5f;
    float aoStrength = 1.0f;
    Vec3 emissiveColor = Vec3(0.0f);
};

class MaterialBuilder {
public:
    MaterialBuilder();
    ~MaterialBuilder();

    MaterialBuilder(const MaterialBuilder&) = delete;
    MaterialBuilder& operator=(const MaterialBuilder&) = delete;
    MaterialBuilder(MaterialBuilder&&) noexcept;
    MaterialBuilder& operator=(MaterialBuilder&&) noexcept;

    MaterialBuilder& withVertexShader(std::string path);
    MaterialBuilder& withFragmentShader(std::string path);
    MaterialBuilder& withTexture(std::string name, const Texture& texture);
    MaterialBuilder& withUniform(std::string name, float value);
    MaterialBuilder& withUniform(std::string name, Vec3 value);
    MaterialBuilder& withUniform(std::string name, Vec4 value);
    MaterialBuilder& withPBRTextures(const PBRTextures& textures);
    MaterialBuilder& withPBRUniforms(const PBRUniforms& uniforms);
    MaterialBuilder& withBlendMode(BlendMode mode);
    MaterialBuilder& withDepthTest(bool enabled);
    MaterialBuilder& withDepthWrite(bool enabled);
    MaterialBuilder& withCullMode(CullMode mode);
    MaterialBuilder& withTopology(Topology topology);
    Material build();

private:
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::vector<std::pair<std::string, const Texture*>> textures;
    std::vector<std::pair<std::string, float>> floatUniforms;
    std::vector<std::pair<std::string, Vec3>> vec3Uniforms;
    std::vector<std::pair<std::string, Vec4>> vec4Uniforms;
    PBRTextures pbrTextures;
    PBRUniforms pbrUniforms;
    BlendMode blendMode = BlendMode::Off;
    bool depthTest = true;
    bool depthWrite = true;
    CullMode cullMode = CullMode::Back;
    Topology topology = Topology::TriangleList;
};

class Material {
public:
    static MaterialBuilder builder();

    std::string vertexShader() const;
    std::string fragmentShader() const;
    Topology topology() const;
    CullMode cullMode() const;
    BlendMode blendMode() const;
    bool depthTest() const;
    bool depthWrite() const;
    std::span<const std::pair<std::string, const Texture*>> textures() const;
    std::span<const std::pair<std::string, float>> floatUniforms() const;
    std::span<const std::pair<std::string, Vec3>> vec3Uniforms() const;
    std::span<const std::pair<std::string, Vec4>> vec4Uniforms() const;
    PBRTextures pbrTextures() const;
    PBRUniforms pbrUniforms() const;

    Material() = default;
    ~Material();
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

private:
    struct Impl;
    struct ImplDeleter {
        void operator()(Impl* p) const;
    };
    std::unique_ptr<Impl, ImplDeleter> pimpl;
    friend class MaterialBuilder;
};

}
