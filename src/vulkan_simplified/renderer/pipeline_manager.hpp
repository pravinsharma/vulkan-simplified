#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <unordered_map>

#include "vks/backend/vulkan_context.hpp"
#include "vks/types.hpp"

namespace vks::renderer {

struct PipelineKey {
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    vks::Topology topology = vks::Topology::TriangleList;
    vks::CullMode cullMode = vks::CullMode::Back;
    vks::BlendMode blendMode = vks::BlendMode::Off;
    bool depthTest = true;
    bool depthWrite = true;

    bool operator==(const PipelineKey& other) const {
        return vertexShaderPath == other.vertexShaderPath &&
               fragmentShaderPath == other.fragmentShaderPath &&
               topology == other.topology &&
               cullMode == other.cullMode &&
               blendMode == other.blendMode &&
               depthTest == other.depthTest &&
               depthWrite == other.depthWrite;
    }
};

struct PipelineKeyHash {
    std::size_t operator()(const PipelineKey& key) const {
        std::size_t h1 = std::hash<std::string>{}(key.vertexShaderPath);
        std::size_t h2 = std::hash<std::string>{}(key.fragmentShaderPath);
        std::size_t h3 = std::hash<int>{}(static_cast<int>(key.topology));
        std::size_t h4 = std::hash<int>{}(static_cast<int>(key.cullMode));
        std::size_t h5 = std::hash<int>{}(static_cast<int>(key.blendMode));
        std::size_t h6 = std::hash<bool>{}(key.depthTest);
        std::size_t h7 = std::hash<bool>{}(key.depthWrite);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6);
    }
};

class PipelineManager {
public:
    explicit PipelineManager(backend::VulkanContext& ctx);
    ~PipelineManager();

    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;

    VkPipelineCache getCache() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;
    VkPipelineLayout getPipelineLayout() const;
    VkPipelineLayout getDefaultPipelineLayout() const;

    VkPipeline getOrCreatePipeline(const PipelineKey& key);

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;

    void createDefaultLayouts();
};

}
