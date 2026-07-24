#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <vulkan/vulkan.h>

namespace vks::backend {
class VulkanContext;
}

namespace vks::renderer {

class RenderGraph {
public:
    struct TextureDesc {
        uint32_t width = 1;
        uint32_t height = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        bool sampled = true;
        VkClearValue clearValue{{0.0f, 0.0f, 0.0f, 0.0f}};
    };

    struct BufferDesc {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    };

    class Resource {
    public:
        enum class Type { Texture, Buffer };
        Type type() const { return type_; }
        uint32_t index() const { return index_; }

        static Resource texture(uint32_t index) { return Resource(Type::Texture, index); }
        static Resource buffer(uint32_t index) { return Resource(Type::Buffer, index); }

    private:
        friend class RenderGraph;
        Resource(Type t, uint32_t i) : type_(t), index_(i) {}
        Type type_;
        uint32_t index_;
    };

    class RenderGraphContext {
    public:
        VkImageView getTextureView(Resource res) const { return graph_.getTextureView(res); }
        VkImage getTextureImage(Resource res) const { return graph_.getTextureImage(res); }
        VkFormat getTextureFormat(Resource res) const { return graph_.getTextureFormat(res); }
        VkExtent2D extent() const { return graph_.extent(); }
        vks::backend::VulkanContext& ctx() const { return graph_.ctx_; }

    private:
        friend class RenderGraph;
        RenderGraphContext(const RenderGraph& graph, VkCommandBuffer cmd)
            : graph_(graph), cmd_(cmd) {}
        const RenderGraph& graph_;
        VkCommandBuffer cmd_;
    };

    class Pass {
    public:
        virtual ~Pass() = default;
        virtual void execute(VkCommandBuffer cmd, RenderGraphContext& ctx) = 0;
        std::span<const Resource> inputs() const { return inputs_; }
        std::span<const Resource> outputs() const { return outputs_; }

    protected:
        void setInputs(std::vector<Resource> in) { inputs_ = std::move(in); }
        void setOutputs(std::vector<Resource> out) { outputs_ = std::move(out); }

    private:
        std::vector<Resource> inputs_;
        std::vector<Resource> outputs_;
    };

    explicit RenderGraph(vks::backend::VulkanContext& ctx, uint32_t width, uint32_t height);
    ~RenderGraph();

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    Resource addTexture(const TextureDesc& desc);
    Resource addBuffer(const BufferDesc& desc);

    void addPass(std::unique_ptr<Pass> pass);
    void addExternalTexture(VkImage image, VkImageView view, VkFormat format);

    bool compile();
    void execute(VkCommandBuffer cmd, uint32_t frameIndex);
    void onResize(uint32_t width, uint32_t height);

    VkImageView getTextureView(Resource res) const;
    VkImage getTextureImage(Resource res) const;
    VkFormat getTextureFormat(Resource res) const;
    VkExtent2D extent() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
    vks::backend::VulkanContext& ctx_;
};

}
