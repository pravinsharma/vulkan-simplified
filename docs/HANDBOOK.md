# VulkanSimplified Handbook

> The canonical guide for using VulkanSimplified in real projects. Every section includes copy-paste-ready examples.

## Table of Contents

1. [Getting Started](#getting-started)
2. [The App Lifecycle](#the-app-lifecycle)
3. [Scene & Entities](#scene--entities)
4. [Materials](#materials)
5. [Shaders](#shaders)
6. [Textures](#textures)
7. [Meshes](#meshes)
8. [Cameras](#cameras)
9. [Frame & Draw Calls](#frame--draw-calls)
10. [Render Graph](#render-graph)
11. [Post-Processing](#post-processing)
12. [Compute Passes](#compute-passes)
13. [Debug & Diagnostics](#debug--diagnostics)
14. [Headless Rendering](#headless-rendering)
15. [Error Handling](#error-handling)
16. [Multithreading](#multithreading)
17. [Platform Notes](#platform-notes)
18. [Advanced Usage](#advanced-usage)

---

## Getting Started

### Requirements

| Requirement | Minimum |
|-------------|---------|
| C++ Standard | C++20 |
| CMake | 3.24+ |
| Vulkan SDK | 1.3.x |
| GPU | Vulkan 1.3 capable |
| Compiler | MSVC 19.40+, GCC 13+, Clang 16+ |

### CMake Integration

```cmake
find_package(VulkanSimplified REQUIRED)
target_link_libraries(MyApp PRIVATE VulkanSimplified)
```

When using vcpkg, the `vcpkg.json` manifest handles transitive dependencies automatically:

```json
{
  "name": "my-app",
  "dependencies": [
    "vulkan-simplified"
  ]
}
```

### Minimal Example

```cpp
#include <vulkan_simplified/vulkan_simplified.hpp>

using namespace vks;

int main() {
    App app({.title = "Hello Vulkan", .width = 1280, .height = 720});

    Mesh sphere = Mesh::load("assets/sphere.obj");
    Material mat = Material::builder()
        .withVertexShader("shaders/basic.vert")
        .withFragmentShader("shaders/basic.frag")
        .build();

    EntityId entity = app.scene().create()
        .withMesh(sphere)
        .withMaterial(mat)
        .withTransform({.position = {0, 0, -5}})
        .commit();

    Camera cam = app.createCamera({.fov = 60, .near = 0.1f, .far = 1000});

    app.run([&](Frame& frame, float dt) {
        frame.setCamera(cam);
        frame.clear(Color::fromRGB(0.05f, 0.05f, 0.06f), ClearFlags::Color | ClearFlags::Depth);
        frame.draw(entity);
    });

    return 0;
}
```

That is the entire rendering loop. The library owns the swapchain, command buffers, synchronization, descriptor sets, and presentation.

---

## The App Lifecycle

### Configuration

```cpp
App::Config cfg = {
    .title = "My Renderer",
    .width = 1920,
    .height = 1080,
    .vsync = true,
    .headless = false
};

App app(cfg);
```

| Field | Default | Description |
|-------|---------|-------------|
| `title` | `"VulkanSimplified App"` | Window title |
| `width` | `1280` | Window width in pixels |
| `height` | `720` | Window height in pixels |
| `vsync` | `true` | Enable vsync via FIFO present mode |
| `headless` | `false` | Disable window; render to offscreen image |

### Main Loop

```cpp
app.run([&](Frame& frame, float dt) {
    // dt = delta time in seconds since last frame
    updateScene(dt);
    frame.setCamera(cam);
    frame.draw(visibleEntities);
});
```

The callback is invoked once per frame. `dt` is useful for animation and time-based updates.

### Window Events

```cpp
app.onResize([&](int w, int h) {
    cam.setAspectRatio(static_cast<float>(w) / static_cast<float>(h));
});

app.onClose([&] {
    saveScreenshot();
});
```

---

## Scene & Entities

### Creating an Entity

```cpp
EntityId id = app.scene().create()
    .withMesh(sphereMesh)
    .withMaterial(pbrMaterial)
    .withTransform({
        .position = {0, 2, -8},
        .rotation = Quat::fromEuler({0, 45_deg, 0}),
        .scale = {2, 2, 2}
    })
    .commit();
```

`EntityId` is a lightweight handle. `InvalidEntity` (`0`) is reserved.

### Transform

```cpp
struct Transform {
    Vec3 position = {0, 0, 0};
    Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
    Vec3 scale = {1, 1, 1};
};
```

```cpp
Entity& e = app.scene().get(id);
e.transform().position = Vec3(1, 0, 0);
e.transform().rotation = Quat::fromEuler({0, 90_deg, 0});
e.transform().scale = Vec3(0.5f, 0.5f, 0.5f);
```

### Querying Entities

```cpp
// Iterate all entities that have both Mesh and Material
auto view = app.scene().query<Mesh, Material>();
for (auto [id, mesh, mat] : view) {
    // id is EntityId, mesh is const Mesh&, mat is const Material&
}
```

Query only entities with a specific material tag or transform property by filtering manually inside the loop.

### Async Queries

```cpp
std::future<std::vector<EntityId>> future = app.scene().queryAsync<Mesh, Material>();
// ... do other work ...
auto ids = future.get();
```

### Destroying Entities

```cpp
app.scene().destroy(id);
// EntityId is now invalid. Do not reuse until confirmed recycled.
```

---

## Materials

### Basic Material

```cpp
Material mat = Material::builder()
    .withVertexShader("shaders/basic.vert")
    .withFragmentShader("shaders/basic.frag")
    .withBlendMode(BlendMode::Off)
    .withDepthTest(true)
    .withDepthWrite(true)
    .withCullMode(CullMode::Back)
    .withTopology(Topology::TriangleList)
    .build();
```

### PBR Material with Textures

```cpp
Texture albedo = Texture::load("textures/brick_albedo.png");
Texture normal = Texture::load("textures/brick_normal.png");
Texture mr     = Texture::load("textures/brick_metallicRoughness.png");

PBRTextures pbrTex;
pbrTex.albedo = &albedo;
pbrTex.normal = &normal;
pbrTex.metallicRoughness = &mr;
pbrTex.ao = nullptr;
pbrTex.emissive = nullptr;

PBRUniforms pbrUni;
pbrUni.metallic = 0.8f;
pbrUni.roughness = 0.4f;
pbrUni.aoStrength = 1.0f;
pbrUni.emissiveColor = Vec3(0.0f);

Material pbrMat = Material::builder()
    .withVertexShader("shaders/pbr.vert")
    .withFragmentShader("shaders/pbr.frag")
    .withPBRTextures(pbrTex)
    .withPBRUniforms(pbrUni)
    .build();
```

### Named Textures

```cpp
Material mat = Material::builder()
    .withVertexShader("shaders/custom.vert")
    .withFragmentShader("shaders/custom.frag")
    .withTexture("shadowMap", shadowTexture)
    .withTexture("envMap", environmentTexture)
    .build();
```

Named textures are bound by the shader's `layout(set = 0, binding = N)` uniform names. The library matches by name at compile time.

### Custom Uniforms

```cpp
Material mat = Material::builder()
    .withFragmentShader("shaders/lighting.frag")
    .withUniform("lightDir", Vec3(0.5f, -1.0f, 0.3f).normalized())
    .withUniform("lightColor", Vec3(1.0f, 0.95f, 0.8f))
    .withUniform("ambientStrength", 0.1f)
    .build();
```

Supported `withUniform` overloads:
- `float`
- `Vec3`
- `Vec4`

### Render State

| Method | Values |
|--------|--------|
| `withBlendMode(BlendMode)` | `Off`, `Alpha`, `Additive` |
| `withDepthTest(bool)` | `true` / `false` |
| `withDepthWrite(bool)` | `true` / `false` |
| `withCullMode(CullMode)` | `Off`, `Front`, `Back` |
| `withTopology(Topology)` | `TriangleList`, `TriangleStrip`, `LineList`, `PointList` |

### Material Properties

```cpp
Material mat = ...;
std::println("vert: {}", mat.vertexShader());
std::println("frag: {}", mat.fragmentShader());
std::println("topology: {}", static_cast<int>(mat.topology()));
std::println("cull: {}", static_cast<int>(mat.cullMode()));
std::println("blend: {}", static_cast<int>(mat.blendMode()));

auto textures = mat.textures();
for (auto [name, tex] : textures) {
    std::println("texture '{}': {}x{}", name, tex->width(), tex->height());
}
```

---

## Shaders

### Shader File Convention

Place shaders in a `shaders/` directory. The library watches for changes and recompiles automatically.

```
shaders/
  basic.vert
  basic.frag
  pbr.vert
  pbr.frag
  post_process_bloom.frag
  particle.comp
```

Supported extensions: `.vert`, `.frag`, `.comp`, `.tesc`, `.tese`, `.geom`

### Hot-Reload

No extra code required. Drop a modified shader file into the watched directory; the library detects the change on the next frame and swaps the shader atomically.

```cpp
// Optional: manual shader replacement via ShaderWatcher
ShaderWatcher watcher;
watcher.watch("shaders/pbr.frag", [&](const Shader& newShader) {
    mat.replaceShader("frag", newShader);
});
```

### Writing Shaders

#### Basic Vertex Shader

```glsl
#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(push_constant) struct Push {
    mat4 model;
    vec4 tintColor;
} push;

layout(set = 0, binding = 0) uniform Camera {
    mat4 viewProjection;
    vec3 viewPosition;
};

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main() {
    vec4 world = push.model * vec4(inPosition, 1.0);
    outWorldPos = world.xyz;
    outNormal = mat3(push.model) * inNormal;
    outUV = inUV;
    gl_Position = viewProjection * world;
}
```

#### Basic Fragment Shader

```glsl
#version 460 core

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform vec3 lightDir;

void main() {
    vec3 n = normalize(inNormal);
    float diff = max(dot(n, normalize(lightDir)), 0.0);
    vec3 tex = texture(diffuseMap, inUV).rgb;
    outColor = vec4(tex * (diff + 0.2), 1.0);
}
```

#### PBR Fragment Shader

```glsl
#version 460 core

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 0, binding = 3) uniform sampler2D aoMap;

layout(set = 0, binding = 4) uniform PBRParams {
    float metallic;
    float roughness;
    float aoStrength;
    vec3 emissiveColor;
};

layout(set = 0, binding = 5) uniform samplerCube envMap;

// ... PBR implementation ...
```

### Shader Compilation Pipeline

1. File watcher detects `.vert`, `.frag`, `.comp` change
2. Library parses for `binding = N` uniforms and `location = N` attributes
3. Compiles to SPIR-V via `glslang` (GLSL) or `dxc` (HLSL/MSL)
4. Caches compiled binary by content hash in `~/.cache/vulkan-simplified/shaders/`

You do not need to invoke any compiler manually. Shader paths in `Material::builder()` are strings; the library resolves them at runtime.

---

## Textures

### Loading a Texture

```cpp
Texture tex = Texture::load("textures/brick.png");
std::println("Loaded {}x{} format={}", tex.width(), tex.height(), static_cast<int>(tex.format()));
```

### Loading with Options

```cpp
Texture hdr = Texture::load("textures/sky.hdr", {
    .format = TextureFormat::R16G16B16A16SFloat,
    .generateMips = true
});
```

### Texture Formats

| Format | Description |
|--------|-------------|
| `RGBA8` | 8-bit per channel, unorm |
| `R16G16B16A16SFloat` | 16-bit float, HDR |
| `BC1` | Block-compressed 4bpp, no alpha |
| `BC3` | Block-compressed 8bpp, with alpha |

### Supported Image Formats

The `stb_image` backend automatically detects: PNG, JPG, BMP, TGA, HDR, PNM.

### Mipmap Generation

```cpp
Texture tex = Texture::load("texture.png", {.generateMips = true});
if (tex.hasMips()) {
    std::println("Mipmaps generated: {} levels", static_cast<int>(std::log2(std::max(tex.width(), tex.height())) + 1));
}
```

---

## Meshes

### Loading from File

```cpp
Mesh sphere = Mesh::load("assets/sphere.obj");
Mesh plane  = Mesh::load("assets/plane.obj");
```

The library uses a built-in OBJ loader. Only triangle geometry is guaranteed; quads are triangulated.

### Creating from Raw Data

```cpp
std::vector<vks::Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {0, 0, 1}, {0, 0}},
    {{ 0.5f, -0.5f, 0.0f}, {0, 0, 1}, {1, 0}},
    {{ 0.5f,  0.5f, 0.0f}, {0, 0, 1}, {1, 1}},
    {{-0.5f,  0.5f, 0.0f}, {0, 0, 1}, {0, 1}},
};

std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

Mesh quad = Mesh::fromVertices(vertices);
// If indices are needed, wrap in a mesh with index buffer via fromVerticesSpan
```

### Vertex Layout

```cpp
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};
```

`position` is required. `normal` and `uv` are optional in the input; missing attributes are zeroed.

### Mesh Bounds

```cpp
Mesh mesh = Mesh::load("assets/character.obj");
BoundingBox bounds = mesh.bounds();
std::println("min: ({}, {}, {})", bounds.min.x, bounds.min.y, bounds.min.z);
std::println("max: ({}, {}, {})", bounds.max.x, bounds.max.y, bounds.max.z);
```

---

## Cameras

### Creating a Camera

```cpp
Camera cam = app.createCamera({
    .fovDegrees = 70.0f,
    .aspectRatio = static_cast<float>(width) / static_cast<float>(height),
    .near = 0.1f,
    .far = 500.0f
});
```

### Look-At

```cpp
cam.lookAt(
    {0.0f, 2.0f, 8.0f},   // eye
    {0.0f, 0.0f, 0.0f},   // target
    {0.0f, 1.0f, 0.0f}    // up
);
```

### Camera Helpers

```cpp
Vec3 pos = cam.position();
Vec3 fwd = cam.forward();
Vec3 up  = cam.up();
Vec3 rgt = cam.right();

Mat4 v = cam.view();
Mat4 p = cam.projection();
Mat4 vp = cam.viewProjection();
```

### Updating Aspect Ratio on Resize

```cpp
app.onResize([&](int w, int h) {
    cam = app.createCamera({
        .fovDegrees = 70.0f,
        .aspectRatio = static_cast<float>(w) / static_cast<float>(h),
        .near = 0.1f,
        .far = 500.0f
    });
});
```

---

## Frame & Draw Calls

### Setting the Camera

```cpp
app.run([&](Frame& frame, float dt) {
    frame.setCamera(cam);
    frame.draw(entities);
});
```

### Clearing

```cpp
frame.clear(Color::fromHex(0x202028FF), ClearFlags::Color | ClearFlags::Depth);
// Or
frame.clear(Color{0.05f, 0.05f, 0.06f, 1.0f}, ClearFlags::Color | ClearFlags::Depth | ClearFlags::Stencil);
```

### Drawing an Entity

```cpp
frame.draw(entityId);
```

### Drawing Multiple Entities

```cpp
std::vector<EntityId> visible = cullFrustum(cam, app.scene());
frame.draw(visible);
```

### Per-Draw Overrides with DrawCall

```cpp
DrawCall call;
call.entity = entityId;
call.modelMatrix = Mat4(1.0f);
call.tintColor = Color::fromRGB(1.0f, 0.8f, 0.2f, 1.0f);
call.roughnessOverride = 0.3f;
call.metallicOverride = 0.9f;

frame.draw(call);
```

```cpp
std::vector<DrawCall> calls;
for (EntityId id : visible) {
    DrawCall c;
    c.entity = id;
    c.modelMatrix = computeModelMatrix(id);
    if (isHighlighted(id)) {
        c.tintColor = Color::fromRGB(1.0f, 0.0f, 0.0f, 1.0f);
    }
    calls.push_back(c);
}
frame.draw(calls);
```

---

## Render Graph

The render graph manages automatic subpass layout, attachment aliasing, and resource lifetime. Use it for custom multi-pass effects.

### Basic Setup

```cpp
#include <vulkan_simplified/render_graph.hpp>

using namespace vks::renderer;

RenderGraph rg(app.vulkanContext(), 1280, 720);

Resource hdrColor = rg.addTexture({
    .width = 1280,
    .height = 720,
    .format = VK_FORMAT_R16G16B16A16_SFLOAT,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sampled = true,
    .clearValue = {{0.0f, 0.0f, 0.0f, 0.0f}}
});

Resource sceneDepth = rg.addTexture({
    .width = 1280,
    .height = 720,
    .format = VK_FORMAT_D32_SFLOAT,
    .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    .sampled = true,
    .clearValue = {{1.0f, 0}}
});

// Add a custom pass
struct MyPass : public RenderGraph::Pass {
    RenderGraph::Resource input_;
    RenderGraph::Resource output_;

    void execute(VkCommandBuffer cmd, RenderGraphContext& ctx) override {
        VkImageView in = ctx.getTextureView(input_);
        VkImageView out = ctx.getTextureView(output_);
        // Bind pipeline, descriptor sets, draw fullscreen quad...
    }
};

rg.addPass(std::make_unique<MyPass>());

if (!rg.compile()) {
    throw std::runtime_error("Render graph compilation failed");
}

// In your render loop
rg.execute(commandBuffer, frameIndex);
```

### External Textures

```cpp
// Import a texture from outside the graph
VkImage externalImage = ...;
VkImageView externalView = ...;
rg.addExternalTexture(externalImage, externalView, VK_FORMAT_R8G8B8A8_UNORM);
```

### Resize Handling

```cpp
app.onResize([&](int w, int h) {
    rg.onResize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
});
```

---

## Post-Processing

### Bloom

```cpp
#include <vulkan_simplified/post_process.hpp>

vks::renderer::BloomPass::Config bloomCfg;
bloomCfg.threshold = 1.0f;
bloomCfg.intensity = 0.5f;
bloomCfg.mipLevels = 5;

vks::renderer::BloomPass bloom(bloomCfg);
bloom.setInput(hdrColor);
bloom.setOutput(bloomOutput);

rg.addPass(std::make_unique<vks::renderer::BloomPass>(bloomCfg));
```

### SSAO

```cpp
vks::renderer::SSAOPass::Config ssaoCfg;
ssaoCfg.kernelSize = 32;
ssaoCfg.radius = 0.5f;
ssaoCfg.bias = 0.025f;

vks::renderer::SSAOPass ssao(ssaoCfg);
ssao.setDepthInput(sceneDepth);
ssao.setNormalInput(sceneNormal);
ssao.setOutput(ssaoOutput);
```

### Tone Mapping

```cpp
vks::renderer::ToneMapPass::Config toneCfg;
toneCfg.exposure = 1.0f;

vks::renderer::ToneMapPass tone(toneCfg);
tone.setInput(hdrColor);
tone.setOutput(swapchainTarget);
```

---

## Compute Passes

```cpp
ComputePass::Builder particles()
    .withInput("particleBuffer", particleBuffer)
    .withCompute("shaders/particle.comp", {.x = 256, .y = 1, .z = 1})
    .build();

app.addCompute(particles);
```

Inside the compute shader:

```glsl
#version 460 core

layout(set = 0, binding = 0) buffer ParticleBuffer {
    vec4 positions[];
    vec4 velocities[];
};

layout(local_size_x = 256) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= positions.length()) return;

    velocities[id].xyz += vec3(0, -9.8, 0) * 0.016;
    positions[id].xyz += velocities[id].xyz * 0.016;

    if (positions[id].y < -10.0) {
        positions[id].y = 10.0;
        velocities[id].y = 0.0;
    }
}
```

---

## Debug & Diagnostics

### Enabling Validation Layers

```cpp
App::Config cfg = {.debug = true};
App app(cfg);

// Or at runtime
DebugLayer::instance().enableValidationLayers(true);
```

### Frame Diagnostics

```cpp
app.onFrameEnd([](FrameDiagnostics& diag) {
    std::println("Draw calls: {}", diag.drawCalls);
    std::println("Triangles: {}", diag.triangleCount);
    std::println("Pipelines: {}", diag.pipelineCount);

    for (const auto& warning : diag.warnings) {
        std::println("Warning: {}", warning);
    }
});
```

### Manual Warnings

```cpp
DebugLayer::instance().pushWarning("Texture 'brick.png' has non-power-of-two dimensions");
```

---

## Headless Rendering

```cpp
App::Config cfg = {
    .headless = true,
    .width = 4096,
    .height = 4096
};

App app(cfg);

Image result = app.renderToImage([&](Frame& f) {
    f.setCamera(cam);
    f.clear(Color{0, 0, 0, 1}, ClearFlags::Color | ClearFlags::Depth);
    f.draw(scene);
});

result.save("render.exr");
```

Headless mode is useful for automated testing, offline rendering, and server-side content generation.

---

## Error Handling

### Fatal Errors

```cpp
try {
    App app({.title = "My App"});
    // ...
} catch (const vks::FatalError& e) {
    std::cerr << "Fatal: " << e.what() << std::endl;
    return EXIT_FAILURE;
}
```

`vks::FatalError` is thrown when the library cannot continue (e.g., no Vulkan-compatible GPU, shader compilation failure with no recovery).

### Validation Errors

Validation errors during shader compilation or pipeline creation are routed through the built-in logger, not thrown. Enable the debug layer to see them:

```cpp
DebugLayer::instance().enableValidationLayers(true);
```

---

## Multithreading

### Thread-Safe Queries

```cpp
std::j Worker([&] {
    auto view = app.scene().query<Mesh, Material>();
    for (auto [id, mesh, mat] : view) {
        // read-only access is safe
        analyzeMaterial(mat);
    }
});

// Main thread continues
updateAnimations(dt);
Worker.join();
```

### Async Queries

```cpp
std::future<std::vector<EntityId>> future = app.scene().queryAsync<Mesh, Material>();

// ... do other work ...

std::vector<EntityId> ids = future.get();
frame.draw(ids);
```

**Important:** Entity mutation (`withMesh`, `withMaterial`, `destroy`) must happen on the main thread. Reads are thread-safe.

---

## Platform Notes

### Windows

```cpp
// No special configuration needed. SDL3 creates a Win32 HWND surface automatically.
App::Config cfg = {.title = "Windows App", .width = 1280, .height = 720};
App app(cfg);
```

### Linux

```bash
# X11 (default)
cmake -DVKS_PLATFORM=XCB ..

# Wayland
cmake -DVKS_PLATFORM=WAYLAND ..
```

The platform is detected at compile time via `VKS_PLATFORM_XCB` or `VKS_PLATFORM_WAYLAND` preprocessor defines.

### macOS

```cpp
// No code changes required. MoltenVK is selected automatically.
App::Config cfg = {.title = "macOS App", .width = 1280, .height = 720};
App app(cfg);
```

Requirements:
- Vulkan SDK installed
- MoltenVK framework in `VULKAN_SDK` or discovered via vcpkg

---

## Advanced Usage

### Escape Hatches

If you need raw Vulkan handles for custom extensions:

```cpp
VulkanDevice& device = app.vulkanDevice();
VulkanContext& ctx = app.vulkanContext();

VkDevice vkDevice = device.get();
VkPhysicalDevice vkPhysical = device.physical();
```

**Warning:** Using raw handles bypasses the library's memory management and synchronization. You are responsible for correctness.

### Custom Allocators

```cpp
Texture tex = Texture::load("huge_atlas.png", {
    .allocator = Allocator::preferVRAM()
});
```

### Extension Detection

```cpp
if (app.hasExtension("VK_KHR_ray_tracing_pipeline")) {
    // Enable ray tracing path
}
```

### Custom Render Passes in the Render Graph

```cpp
struct CustomPass : public RenderGraph::Pass {
    RenderGraph::Resource input_;
    RenderGraph::Resource output_;

    void execute(VkCommandBuffer cmd, RenderGraphContext& ctx) override {
        VkImageView in = ctx.getTextureView(input_);
        VkImageView out = ctx.getTextureView(output_);
        VkExtent2D ext = ctx.extent();

        // Record custom Vulkan commands here
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        vkCmdDraw(cmd, 3, 1, 0, 0);
    }
};
```

### Shader Variant Management

For large projects needing many shader permutations, organize shaders by convention:

```
shaders/
  pbr/
    base.vert
    base.frag
    skinning.vert
    instancing.vert
```

Use the `MaterialBuilder` to select variants:

```cpp
Material skinMat = Material::builder()
    .withVertexShader("shaders/pbr/skinning.vert")
    .withFragmentShader("shaders/pbr/base.frag")
    .build();
```

### Debug Markers

```cpp
app.beginDebugRegion("Shadow Pass");
frame.draw(shadowCasters);
app.endDebugRegion();

app.insertDebugMarker("Gbuffer Pass");
frame.draw(gbufferEntities);
app.insertDebugMarker("Lighting Pass");
```

---

## Common Patterns

### Loading a Scene

```cpp
std::vector<Entity> loadScene(App& app, const std::string& path) {
    std::vector<Entity> entities;
    for (const auto& node : parseScene(path)) {
        Mesh mesh = Mesh::load(node.meshPath);
        Material mat = Material::builder()
            .withVertexShader("shaders/pbr.vert")
            .withFragmentShader("shaders/pbr.frag")
            .withPBRTextures({
                .albedo = &Texture::load(node.albedo),
                .normal = &Texture::load(node.normal),
                .metallicRoughness = &Texture::load(node.mr)
            })
            .build();

        entities.push_back(app.scene().create()
            .withMesh(mesh)
            .withMaterial(mat)
            .withTransform(node.transform)
            .commit());
    }
    return entities;
}
```

### Instancing via DrawCall

```cpp
std::vector<DrawCall> instances;
Mat4 base = Mat4(1.0f);

for (int i = 0; i < 1000; ++i) {
    DrawCall call;
    call.entity = prototype;
    call.modelMatrix = base * glm::translate(Vec3(i % 10, i / 10, 0));
    instances.push_back(call);
}

frame.draw(instances);
```

### Saving a Screenshot

```cpp
Image screenshot = app.renderToImage([&](Frame& f) {
    f.setCamera(cam);
    f.clear(Color{0, 0, 0, 1}, ClearFlags::Color | ClearFlags::Depth);
    f.draw(entities);
});

screenshot.save("screenshot.png");
```

### Animated Object

```cpp
struct Animated {
    EntityId id;
    Vec3 velocity;
};

std::vector<Animated> objects;

app.run([&](Frame& frame, float dt) {
    for (auto& obj : objects) {
        obj.velocity.y -= 9.8f * dt;
        app.scene().get(obj.id).transform().position += obj.velocity * dt;

        if (app.scene().get(obj.id).transform().position.y < -5.0f) {
            obj.velocity.y = 5.0f;
            app.scene().get(obj.id).transform().position = Vec3(0, 5, 0);
        }
    }

    frame.setCamera(cam);
    for (auto& obj : objects) {
        frame.draw(obj.id);
    }
});
```

---

## Troubleshooting

### Shader Fails to Compile

1. Ensure shader files are in the working directory or provide absolute paths
2. Check that `binding = N` numbers match between shader and `Material` uniforms/textures
3. Enable debug layer: `DebugLayer::instance().enableValidationLayers(true)`

### Texture Appears Black

- Verify the texture path is correct relative to the executable working directory
- Check that the shader sampler name matches `withTexture("name", tex)` exactly
- Confirm texture format is supported for the target GPU

### Performance

- Use `queryAsync` for expensive scene queries
- Batch `DrawCall`s into a single `frame.draw(std::span<const DrawCall>)` to reduce CPU overhead
- Prefer `RGBA8` and `BC1` textures when HDR is not needed
