#include "vulkan_simplified/material.hpp"

#include "vulkan_simplified/texture.hpp"

#include <span>
#include <stdexcept>
#include <string>

namespace {

inline std::string resolveShaderName(const std::string& path) {
    if (!path.empty() && path[0] != '/') {
        return "shaders/" + path;
    }
    return path;
}

}

namespace vks {

struct Material::Impl {
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

MaterialBuilder::MaterialBuilder() = default;

MaterialBuilder::~MaterialBuilder() = default;

MaterialBuilder::MaterialBuilder(MaterialBuilder&&) noexcept = default;
MaterialBuilder& MaterialBuilder::operator=(MaterialBuilder&&) noexcept = default;

Material::~Material() = default;

void Material::ImplDeleter::operator()(Impl* p) const {
    delete p;
}

MaterialBuilder& MaterialBuilder::withVertexShader(std::string path) {
    vertexShaderPath = resolveShaderName(std::move(path));
    return *this;
}

MaterialBuilder& MaterialBuilder::withFragmentShader(std::string path) {
    fragmentShaderPath = resolveShaderName(std::move(path));
    return *this;
}

MaterialBuilder& MaterialBuilder::withTexture(std::string name, const Texture& texture) {
    textures.emplace_back(std::move(name), &texture);
    return *this;
}

MaterialBuilder& MaterialBuilder::withUniform(std::string name, float value) {
    floatUniforms.emplace_back(std::move(name), value);
    return *this;
}

MaterialBuilder& MaterialBuilder::withUniform(std::string name, Vec3 value) {
    vec3Uniforms.emplace_back(std::move(name), value);
    return *this;
}

MaterialBuilder& MaterialBuilder::withUniform(std::string name, Vec4 value) {
    vec4Uniforms.emplace_back(std::move(name), value);
    return *this;
}

MaterialBuilder& MaterialBuilder::withPBRTextures(const PBRTextures& textures) {
    pbrTextures = textures;
    return *this;
}

MaterialBuilder& MaterialBuilder::withPBRUniforms(const PBRUniforms& uniforms) {
    pbrUniforms = uniforms;
    return *this;
}

MaterialBuilder& MaterialBuilder::withBlendMode(BlendMode mode) {
    blendMode = mode;
    return *this;
}

MaterialBuilder& MaterialBuilder::withDepthTest(bool enabled) {
    depthTest = enabled;
    return *this;
}

MaterialBuilder& MaterialBuilder::withDepthWrite(bool enabled) {
    depthWrite = enabled;
    return *this;
}

MaterialBuilder& MaterialBuilder::withCullMode(CullMode mode) {
    cullMode = mode;
    return *this;
}

MaterialBuilder& MaterialBuilder::withTopology(Topology topo) {
    topology = topo;
    return *this;
}

Material MaterialBuilder::build() {
    Material mat;
    mat.pimpl = std::unique_ptr<Material::Impl, Material::ImplDeleter>(new Material::Impl);
    mat.pimpl->vertexShaderPath = vertexShaderPath;
    mat.pimpl->fragmentShaderPath = fragmentShaderPath;
    mat.pimpl->textures = textures;
    mat.pimpl->floatUniforms = floatUniforms;
    mat.pimpl->vec3Uniforms = vec3Uniforms;
    mat.pimpl->vec4Uniforms = vec4Uniforms;
    mat.pimpl->pbrTextures = pbrTextures;
    mat.pimpl->pbrUniforms = pbrUniforms;
    mat.pimpl->blendMode = blendMode;
    mat.pimpl->depthTest = depthTest;
    mat.pimpl->depthWrite = depthWrite;
    mat.pimpl->cullMode = cullMode;
    mat.pimpl->topology = topology;
    if (mat.pimpl->vertexShaderPath.empty() || mat.pimpl->fragmentShaderPath.empty()) {
        throw std::runtime_error("Material requires both vertex and fragment shader paths");
    }
    return mat;
}

MaterialBuilder Material::builder() {
    return MaterialBuilder{};
}

std::string Material::vertexShader() const {
    return pimpl ? pimpl->vertexShaderPath : std::string{};
}

std::string Material::fragmentShader() const {
    return pimpl ? pimpl->fragmentShaderPath : std::string{};
}

Topology Material::topology() const {
    return pimpl ? pimpl->topology : Topology::TriangleList;
}

CullMode Material::cullMode() const {
    return pimpl ? pimpl->cullMode : CullMode::Back;
}

BlendMode Material::blendMode() const {
    return pimpl ? pimpl->blendMode : BlendMode::Off;
}

bool Material::depthTest() const {
    return pimpl ? pimpl->depthTest : true;
}

bool Material::depthWrite() const {
    return pimpl ? pimpl->depthWrite : true;
}

std::span<const std::pair<std::string, const Texture*>> Material::textures() const {
    if (!pimpl) return {};
    return std::span<const std::pair<std::string, const Texture*>>(pimpl->textures.data(), pimpl->textures.size());
}

std::span<const std::pair<std::string, float>> Material::floatUniforms() const {
    if (!pimpl) return {};
    return std::span<const std::pair<std::string, float>>(pimpl->floatUniforms.data(), pimpl->floatUniforms.size());
}

std::span<const std::pair<std::string, Vec3>> Material::vec3Uniforms() const {
    if (!pimpl) return {};
    return std::span<const std::pair<std::string, Vec3>>(pimpl->vec3Uniforms.data(), pimpl->vec3Uniforms.size());
}

std::span<const std::pair<std::string, Vec4>> Material::vec4Uniforms() const {
    if (!pimpl) return {};
    return std::span<const std::pair<std::string, Vec4>>(pimpl->vec4Uniforms.data(), pimpl->vec4Uniforms.size());
}

PBRTextures Material::pbrTextures() const {
    if (!pimpl) return {};
    return pimpl->pbrTextures;
}

PBRUniforms Material::pbrUniforms() const {
    if (!pimpl) return {};
    return pimpl->pbrUniforms;
}

}
