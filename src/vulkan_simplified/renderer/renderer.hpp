#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include "vulkan_simplified/material.hpp"
#include "vulkan_simplified/mesh.hpp"
#include "vulkan_simplified/scene_entity.hpp"
#include "vulkan_simplified/frame.hpp"
#include "vulkan_simplified/types.hpp"

#include "vulkan_simplified/backend/vulkan_context.hpp"
#include "vulkan_simplified/renderer/resource_manager.hpp"
#include "vulkan_simplified/renderer/pipeline_manager.hpp"

namespace vks::renderer {

class PipelineManager;
class ResourceManager;

using MaterialHandle = uint32_t;

struct DrawRecord;

struct FrameSubmitData {
    Mat4 viewProjection = Mat4(1.0f);
    Vec3 viewPosition = Vec3(0.0f);
    Vec4 clearColor = {0.05f, 0.05f, 0.1f, 1.0f};
    int clearFlags = static_cast<int>(vks::ClearFlags::Color) | static_cast<int>(vks::ClearFlags::Depth);
    std::span<const DrawRecord> drawRecords;
    class vks::Scene* scene = nullptr;
};

struct MaterialRecord {
    int id = -1;
    Topology topology = Topology::TriangleList;
    CullMode cullMode = CullMode::Back;
    BlendMode blendMode = BlendMode::Off;
    bool depthTest = true;
    bool depthWrite = true;
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    std::vector<GpuTexture> gpuTextures;
    VkPipeline gpuPipeline = VK_NULL_HANDLE;
};

class ForwardRenderer {
public:
    struct Config {
        bool vsync = true;
        uint32_t frameOverlap = 3;
    };

    explicit ForwardRenderer(class backend::VulkanContext* ctx);
    ~ForwardRenderer();

    ForwardRenderer(const ForwardRenderer&) = delete;
    ForwardRenderer& operator=(const ForwardRenderer&) = delete;

    bool init(const Config& cfg);
    void destroy();

    void setScene(class Scene* scene);

    MaterialHandle registerMaterial(const class Material& material);
    void unregisterMaterial(MaterialHandle handle);

    void submitFrame(const FrameSubmitData& data);
    void onResize(uint32_t width, uint32_t height);

    PipelineManager* pipelineManager() const { return pipelineManager_.get(); }
    ResourceManager* resourceManager() const { return resourceManager_.get(); }
    class backend::VulkanContext* context() { return ctx_; }

private:
    struct Impl;
    std::unique_ptr<class Impl> pimpl;
    std::unique_ptr<PipelineManager> pipelineManager_;
    std::unique_ptr<ResourceManager> resourceManager_;
    class backend::VulkanContext* ctx_ = nullptr;
};

struct DrawRecord {
    EntityId entity;
    Mat4 modelMatrix = Mat4(1.0f);
    std::optional<Color> tintColor;
    std::optional<float> roughnessOverride;
    std::optional<float> metallicOverride;
};

}
