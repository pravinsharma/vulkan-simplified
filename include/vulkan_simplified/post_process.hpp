#pragma once

#include <memory>
#include <vector>

#include "vulkan_simplified/render_graph.hpp"

#include <vulkan/vulkan.h>

namespace vks::renderer {

class PostProcessPass {
public:
    virtual ~PostProcessPass() = default;
    virtual void execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) = 0;
};

class BloomPass : public RenderGraph::Pass {
public:
    struct Config {
        float threshold = 1.0f;
        float intensity = 0.5f;
        uint32_t mipLevels = 5;
    };

    explicit BloomPass(const Config& cfg);
    ~BloomPass();

    void setInput(RenderGraph::Resource input);
    void setOutput(RenderGraph::Resource output);
    void execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) override;

private:
    Config cfg_;
    RenderGraph::Resource input_;
    RenderGraph::Resource output_;
};

class SSAOPass : public RenderGraph::Pass {
public:
    struct Config {
        uint32_t kernelSize = 32;
        float radius = 0.5f;
        float bias = 0.025f;
    };

    explicit SSAOPass(const Config& cfg);
    ~SSAOPass();

    void setDepthInput(RenderGraph::Resource depth);
    void setNormalInput(RenderGraph::Resource normal);
    void setOutput(RenderGraph::Resource output);
    void execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) override;

private:
    Config cfg_;
    RenderGraph::Resource depth_;
    RenderGraph::Resource normal_;
    RenderGraph::Resource output_;
};

class ToneMapPass : public RenderGraph::Pass {
public:
    struct Config {
        float exposure = 1.0f;
    };

    explicit ToneMapPass(const Config& cfg);
    ~ToneMapPass();

    void setInput(RenderGraph::Resource input);
    void setOutput(RenderGraph::Resource output);
    void execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) override;

private:
    Config cfg_;
    RenderGraph::Resource input_;
    RenderGraph::Resource output_;
};

}
