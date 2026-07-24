#include "vulkan_simplified/post_process.hpp"

#include "vulkan_simplified/backend/vulkan_context.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

namespace vks::renderer {

namespace {

VkPipeline createFullscreenPipeline(backend::VulkanContext& ctx, VkPipelineLayout layout, VkRenderPass renderPass, VkShaderModule fragModule) {
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = ctx.createShaderModule(std::vector<uint32_t>{
        0x07230203, 0x00010000, 0x00080001, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
        0x00000001, 0x4c534c47, 0x6474732e, 0x30353e29, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
        0x000a0001, 0x00000000, 0x0000005e, 0x6e696c73, 0x00000000, 0x0000000e, 0x00000015, 0x0000001e,
        0x4e48036e, 0x00000000, 0x0000000b, 0x0000001e, 0x4f46036e, 0x00f70000, 0x0000001d, 0x00000008,
        0x00000023, 0x00000000, 0x00000004, 0x00000024, 0x00000000, 0x00000003, 0x00000025, 0x00000000,
        0x00000003, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e696c56, 0x00000000,
        0x00030005, 0x00000009, 0x00000000, 0x00040005, 0x0000000d, 0x6f6c6f43, 0x00000072, 0x00030005,
        0x00000014, 0x00000000, 0x00040005, 0x00000020, 0x6f6c6f43, 0x00000070, 0x00050005, 0x00000025,
        0x6f724654, 0x6f6c6f43, 0x00000073, 0x00040005, 0x0000002a, 0x6f724654, 0x6f6c6f43, 0x00000074,
        0x00030005, 0x00000030, 0x00000000, 0x00040005, 0x00000031, 0x565f6c67, 0x00000072, 0x00050005,
        0x00000036, 0x500f6c67, 0x6f6c6f43, 0x00000070, 0x00050005, 0x0000003b, 0x6f724654, 0x6f6c6f43,
        0x00000073, 0x00040006, 0x00000043, 0x00000000, 0x00000004, 0x00030006, 0x00000014, 0x00000000,
        0x00050048, 0x00000014, 0x00000000, 0x0000000b, 0x00000000, 0x00030047, 0x00000014, 0x00000002,
        0x00040047, 0x00000025, 0x0000000b, 0x0000002a, 0x00040047, 0x0000002a, 0x0000001e, 0x00000000,
        0x00040047, 0x00000031, 0x0000000b, 0x00000036, 0x00040047, 0x00000036, 0x0000001e, 0x00000000,
        0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020,
        0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00040020, 0x00000008, 0x00000006, 0x00000007,
        0x00040020, 0x0000000b, 0x00000006, 0x00000007, 0x0004002b, 0x00000006, 0x0000000c, 0x00000000,
        0x0004002b, 0x00000006, 0x0000000f, 0x3f800000, 0x0004002b, 0x00000006, 0x00000013, 0x00000000,
        0x00040020, 0x00000015, 0x00000001, 0x00000007, 0x00040020, 0x0000001a, 0x00000001, 0x00000007,
        0x0004002b, 0x00000006, 0x00000020, 0x3f800000, 0x0004002b, 0x00000006, 0x00000025, 0x00000000,
        0x0004002b, 0x00000006, 0x0000002a, 0x3f800000, 0x0004002b, 0x00000006, 0x00000030, 0x00000000,
        0x0004002b, 0x00000006, 0x00000031, 0xbf800000, 0x0004002b, 0x00000006, 0x00000036, 0x3f800000,
        0x00040020, 0x0000003b, 0x00000001, 0x00000007, 0x0004003b, 0x00000015, 0x0000003c, 0x0000000b,
        0x00040020, 0x0000003e, 0x00000001, 0x00000007, 0x00050036, 0x00000002, 0x00000003, 0x00000000,
        0x00000004, 0x000200f8, 0x00000005, 0x0004003b, 0x00000008, 0x0000003f, 0x0000000c, 0x0004003b,
        0x00000008, 0x00000040, 0x0000000f, 0x0004003b, 0x00000008, 0x00000041, 0x00000013, 0x000700f8,
        0x0000003c, 0x00000004, 0x00000041, 0x0000003f, 0x00000040, 0x00000002, 0x000300f7, 0x0000000d,
        0x00000000, 0x000400fa, 0x0000000e, 0x0000000d, 0x00000005, 0x0006000c, 0x0000000e, 0x00000014,
        0x00000018, 0x00000018, 0x00000018, 0x0006000c, 0x00000014, 0x00000015, 0x00000001, 0x00000000,
        0x00000001, 0x000300f7, 0x00000017, 0x00000000, 0x000400fa, 0x00000018, 0x00000017, 0x00000002,
        0x00050081, 0x00000007, 0x0000001c, 0x00000015, 0x0000003c, 0x00050081, 0x00000007, 0x0000001d,
        0x00000015, 0x00000040, 0x000300f7, 0x0000001f, 0x00000000, 0x000400fa, 0x00000020, 0x0000001f,
        0x00000005, 0x0006000c, 0x00000020, 0x00000025, 0x00000029, 0x00000029, 0x00000029, 0x0006000c,
        0x00000025, 0x0000001a, 0x00000001, 0x00000000, 0x00000001, 0x000300f7, 0x00000028, 0x00000000,
        0x000400fa, 0x00000029, 0x00000028, 0x00000002, 0x00050081, 0x00000007, 0x0000002d, 0x0000001a,
        0x0000003f, 0x00050081, 0x00000007, 0x0000002e, 0x0000001a, 0x00000040, 0x000300f7, 0x00000030,
        0x00000000, 0x000400fa, 0x00000031, 0x00000030, 0x00000005, 0x0006000c, 0x00000031, 0x00000036,
        0x0000003a, 0x0000003a, 0x0000003a, 0x0006000c, 0x00000036, 0x0000003b, 0x00000001, 0x00000000,
        0x00000001, 0x000300f7, 0x00000039, 0x00000000, 0x000400fa, 0x0000003a, 0x00000039, 0x00000002,
        0x00050081, 0x00000007, 0x0000003f, 0x0000003b, 0x0000003c, 0x00050081, 0x00000007, 0x00000040,
        0x0000003b, 0x00000041, 0x000200f9, 0x00000043, 0x000300f7, 0x00000044, 0x00000000, 0x000400fa,
        0x00000045, 0x00000044, 0x00000002, 0x00050081, 0x00000007, 0x00000046, 0x00000008, 0x0000003f,
        0x00050081, 0x00000007, 0x00000047, 0x00000008, 0x00000040, 0x000200f9, 0x00000048, 0x000200f8,
        0x00000048, 0x000300f7, 0x00000049, 0x00000000, 0x000400fa, 0x0000004a, 0x00000049, 0x00000002,
        0x00050081, 0x00000007, 0x0000004b, 0x0000001a, 0x0000003f, 0x00050081, 0x00000007, 0x0000004c,
        0x0000001a, 0x00000040, 0x000200f9, 0x0000004d, 0x000200f8, 0x0000004d, 0x000300f7, 0x0000004e,
        0x00000000, 0x000400fa, 0x0000004f, 0x0000004e, 0x00000002, 0x00050081, 0x00000007, 0x00000050,
        0x0000003b, 0x0000003c, 0x00050081, 0x00000007, 0x00000051, 0x0000003b, 0x00000041, 0x000200f9,
        0x00000052, 0x000200f8, 0x00000052, 0x000100fd, 0x00010038
    });
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1;
    vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState blend{};
    blend.blendEnable = VK_FALSE;
    blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &blend;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount = 2;
    ci.pStages = stages;
    ci.pVertexInputState = &vi;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState = &vp;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState = &ms;
    ci.pColorBlendState = &cb;
    ci.pDepthStencilState = &ds;
    ci.layout = layout;
    ci.renderPass = renderPass;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(ctx.device(), VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return pipeline;
}

}

BloomPass::BloomPass(const Config& cfg)
    : cfg_(cfg)
    , input_(RenderGraph::Resource::texture(0))
    , output_(RenderGraph::Resource::texture(0))
{
}

BloomPass::~BloomPass() = default;

void BloomPass::setInput(RenderGraph::Resource input) {
    input_ = input;
}

void BloomPass::setOutput(RenderGraph::Resource output) {
    output_ = output;
}

void BloomPass::execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) {
    (void)cmd;
    (void)ctx;
}

SSAOPass::SSAOPass(const Config& cfg)
    : cfg_(cfg)
    , depth_(RenderGraph::Resource::texture(0))
    , normal_(RenderGraph::Resource::texture(0))
    , output_(RenderGraph::Resource::texture(0))
{
}

SSAOPass::~SSAOPass() = default;

void SSAOPass::setDepthInput(RenderGraph::Resource depth) {
    depth_ = depth;
}

void SSAOPass::setNormalInput(RenderGraph::Resource normal) {
    normal_ = normal;
}

void SSAOPass::setOutput(RenderGraph::Resource output) {
    output_ = output;
}

void SSAOPass::execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) {
    (void)cmd;
    (void)ctx;
}

ToneMapPass::ToneMapPass(const Config& cfg)
    : cfg_(cfg)
    , input_(RenderGraph::Resource::texture(0))
    , output_(RenderGraph::Resource::texture(0))
{
}

ToneMapPass::~ToneMapPass() = default;

void ToneMapPass::setInput(RenderGraph::Resource input) {
    input_ = input;
}

void ToneMapPass::setOutput(RenderGraph::Resource output) {
    output_ = output;
}

void ToneMapPass::execute(VkCommandBuffer cmd, RenderGraph::RenderGraphContext& ctx) {
    (void)cmd;
    (void)ctx;
}

}