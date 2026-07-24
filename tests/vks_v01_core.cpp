#include <catch2/catch_test_macros.hpp>
#include <vks/vks.hpp>

TEST_CASE("v0.1 types compile", "[compile]") {
    vks::Vec3 v{1.0f, 2.0f, 3.0f};
    vks::Mat4 m = vks::Mat4(1.0f);
    vks::Color c(1.0f, 0.0f, 0.0f, 1.0f);
    REQUIRE(m[0][0] == 1.0f);
}

TEST_CASE("material builder compiles", "[compile]") {
    auto builder = vks::Material::builder();
    auto mat = builder
                   .withVertexShader("basic.vert")
                   .withFragmentShader("basic.frag")
                   .withCullMode(vks::CullMode::Back)
                   .withBlendMode(vks::BlendMode::Off)
                   .withDepthTest(true)
                   .withDepthWrite(true)
                   .build();
    REQUIRE(mat.vertexShader().find("basic.vert") != std::string::npos);
    REQUIRE(mat.fragmentShader().find("basic.frag") != std::string::npos);
    REQUIRE(mat.cullMode() == vks::CullMode::Back);
}

TEST_CASE("v0.2 PBR material DSL compiles", "[compile]") {
    vks::PBRTextures pbrTex;
    pbrTex.albedo = nullptr;
    pbrTex.normal = nullptr;
    pbrTex.metallicRoughness = nullptr;
    pbrTex.ao = nullptr;
    pbrTex.emissive = nullptr;

    vks::PBRUniforms pbrUni;
    pbrUni.metallic = 0.5f;
    pbrUni.roughness = 0.3f;
    pbrUni.aoStrength = 0.8f;
    pbrUni.emissiveColor = vks::Vec3(0.1f, 0.2f, 0.3f);

    auto builder = vks::Material::builder();
    auto mat = builder
                   .withVertexShader("basic.vert")
                   .withFragmentShader("basic.frag")
                   .withPBRTextures(pbrTex)
                   .withPBRUniforms(pbrUni)
                   .build();

    REQUIRE(mat.pbrTextures().albedo == nullptr);
    REQUIRE(mat.pbrUniforms().metallic == 0.5f);
    REQUIRE(mat.pbrUniforms().roughness == 0.3f);
}

TEST_CASE("v0.2 DrawCall per-draw overrides compile", "[compile]") {
    vks::DrawCall call{};
    call.entity = 1;
    call.modelMatrix = vks::Mat4(1.0f);
    call.tintColor = vks::Color(1.0f, 0.0f, 0.0f, 1.0f);
    call.roughnessOverride = 0.5f;
    call.metallicOverride = 0.8f;

    REQUIRE(call.entity == 1);
    REQUIRE(call.tintColor.has_value());
    REQUIRE(call.roughnessOverride.has_value());
    REQUIRE(call.metallicOverride.has_value());
}

TEST_CASE("v0.2 DebugLayer compiles", "[compile]") {
    vks::DebugLayer::instance().enableValidationLayers(true);
    REQUIRE(vks::DebugLayer::instance().isEnabled() == true);
    vks::DebugLayer::instance().pushWarning("test warning");
    vks::DebugLayer::instance().reset();
    REQUIRE(vks::DebugLayer::instance().isEnabled() == true);
}

TEST_CASE("v0.3 RenderGraph compiles", "[compile]") {
    vks::renderer::RenderGraph::TextureDesc texDesc;
    texDesc.width = 1280;
    texDesc.height = 720;
    texDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
    texDesc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    vks::renderer::RenderGraph::BufferDesc bufDesc;
    bufDesc.size = 256;
    bufDesc.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    auto tex = vks::renderer::RenderGraph::Resource::texture(0);
    auto buf = vks::renderer::RenderGraph::Resource::buffer(0);

    REQUIRE(tex.type() == vks::renderer::RenderGraph::Resource::Type::Texture);
    REQUIRE(buf.type() == vks::renderer::RenderGraph::Resource::Type::Buffer);
    REQUIRE(tex.index() == 0);
    REQUIRE(buf.index() == 0);
}

TEST_CASE("v0.3 Post-process passes compile", "[compile]") {
    vks::renderer::BloomPass::Config bloomCfg;
    bloomCfg.threshold = 1.0f;
    bloomCfg.intensity = 0.5f;

    vks::renderer::SSAOPass::Config ssaoCfg;
    ssaoCfg.kernelSize = 32;
    ssaoCfg.radius = 0.5f;

    vks::renderer::ToneMapPass::Config toneCfg;
    toneCfg.exposure = 1.0f;

    vks::renderer::BloomPass bloom(bloomCfg);
    vks::renderer::SSAOPass ssao(ssaoCfg);
    vks::renderer::ToneMapPass tone(toneCfg);

    auto tex = vks::renderer::RenderGraph::Resource::texture(0);
    bloom.setInput(tex);
    bloom.setOutput(tex);
    ssao.setDepthInput(tex);
    ssao.setNormalInput(tex);
    ssao.setOutput(tex);
    tone.setInput(tex);
    tone.setOutput(tex);
}

TEST_CASE("v0.3 Headless App config compiles", "[compile]") {
    vks::App::Config cfg;
    cfg.headless = true;
    cfg.title = "Test";
    cfg.width = 640;
    cfg.height = 480;
    REQUIRE(cfg.headless == true);
    REQUIRE(cfg.title == "Test");
}
