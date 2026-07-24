# VulkanSimplified

A high-level graphics abstraction that hides all Vulkan complexity. Developers describe **what** to draw; the library owns **how** it gets rendered.

## Core Philosophy

- Zero Vulkan concepts in your workflow: no `VkDevice`, no `VkCommandBuffer`, no `VkPipeline`.
- Declarative scene description. You define meshes, materials, cameras, and draw calls; the renderer handles swapchains, synchronization, memory, and driver quirks.
- Hot-reloadable shaders without touching GLSL/SPIR-V build chains.
- Cross-platform: Windows, Linux, macOS (via MoltenVK).

## Quick Example

```cpp
#include <vulkan_simplified/vulkan_simplified.hpp>

using namespace vks;

int main() {
    App app({.title = "My App", .width = 1280, .height = 720});

    Mesh sphere = Mesh::load("sphere.obj");
    Material mat = Material::builder()
        .withFragmentShader("shaders/pbr.frag")
        .withTexture("albedo", Texture::load("brick_albedo.png"))
        .withUniform("lightDir", Vec3{0.5f, -1.0f, 0.3f}.normalized())
        .build();

    Entity entity = app.scene().create()
        .withMesh(sphere)
        .withMaterial(mat)
        .withTransform(Transform{.position = {0, 0, -5}})
        .commit();

    Camera cam = app.createCamera({.fov = 60, .near = 0.1f, .far = 1000});

    app.run([&](Frame& frame, float dt) {
        frame.setCamera(cam);
        frame.draw(entity);
    });

    return 0;
}
```

That is the entire rendering loop. The library manages the render pass, descriptor sets, command buffers, synchronization objects, and present.

<!-- TOC -->

## Installation

### CMake

```bash
git submodule add https://github.com/your-org/vulkan-simplified third_party/vulkan-simplified
```

```cmake
add_subdirectory(third_party/vulkan-simplified)
target_link_libraries(MyApp PRIVATE VulkanSimplified)
```

### Supported Versions

| Component | Minimum |
|-----------|---------|
| C++ Standard | C++20 |
| Vulkan SDK | 1.3.x |
| GPU | Vulkan 1.3 capable |

## Concepts

### App

The `App` object owns the window, renderer, and scene. It is created once at startup and destroyed at shutdown.

### Scene & Entity

A `Scene` is a container of `Entity` objects. Each entity combines a mesh, a material, and a transform. Entities are **immutable** once created; modify their transform via `TransformComponent`.

Example:

```cpp
EntityId id = app.scene().create()
    .withMesh(planeMesh)
    .withMaterial(gridMaterial)
    .withTransform({.position = {0, 0, 0}, .scale = {10, 10, 1}})
    .commit();

app.scene().get(id).transform().rotation = Quat::fromEuler({0, 45_deg, 0});
```

### Material

Defines the appearance of an entity. Built declaratively:

```cpp
Material mat = Material::builder()
    .withVertexShader("shaders/simple.vert")
    .withFragmentShader("shaders/diffuse.frag")
    .withTexture("diffuseMap", Texture::load("wood.png"))
    .withUniform("tint", Vec4{1, 0.9f, 0.8f, 1})
    .withBlendMode(BlendMode::Alpha)
    .build();
```

Supported features:
- `withDepthTest(bool)` / `withDepthWrite(bool)`
- `withCullMode(CullMode)` — `Off`, `Front`, `Back`
- `withTopology(Topology)` — `TriangleList`, `LineStrip`, etc.
- `withPushConstant("params", struct { ... })`

### Camera

```cpp
Camera cam = app.createCamera(CameraSettings{
    .fov = 70_deg,
    .aspectRatio = static_cast<float>(width) / static_cast<float>(height),
    .near = 0.1f,
    .far = 500.0f
});

// Update per-frame
cam.lookAt({0, 2, 8}, {0, 0, 0}, {0, 1, 0});
```

### Frame

Passed to your per-frame callback. You set the camera and issue draw commands.

```cpp
app.run([&](Frame& frame, float dt) {
    frame.setCamera(cam);
    frame.clear(Color{0.05f, 0.05f, 0.06f, 1}, ClearFlags::Color | ClearFlags::Depth);

    frame.draw(visibleEntities);
});
```

## Shaders

Shaders are auto-compiled. Drop `.vert`, `.frag`, `.comp` files in a `shaders/` folder and the library detects changes at runtime. Supported backends: GLSL, HLSL, MSL.

Uniforms are bound by name from the `Material` builder. You never touch descriptor set layouts.

Example fragment shader:

```glsl
#version 460
layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(set = 0, binding = 0) uniform sampler2D diffuseMap;
layout(set = 0, binding = 1) uniform vec3 lightDir;

layout(location = 0) out vec4 outColor;

void main() {
    float d = max(dot(normalize(inNormal), normalize(lightDir)), 0.0);
    outColor = texture(diffuseMap, inUV) * (d + 0.2);
}
```

## Texture & Image Loading

```cpp
Texture tex = Texture::load("textures/brick.png");
// options
Texture tex = Texture::load("textures/hdr.hdr", {
    .format = TextureFormat::R16G16B16A16SFloat,
    .generateMips = true
});
```

Formats: `RGBA8`, `R16G16B16A16SFloat`, `BC1`, `BC3`, `DXT5`.

## Render Passes & Post-Processing

For advanced use cases, define custom render passes. The library handles attachment layouts and subpass dependencies.

```cpp
auto blurPass = RenderPass::builder()
    .withInput("sceneColor")
    .withOutput("bloom", {.width = app.width() / 2, .height = app.height() / 2})
    .withCompute("shaders/blur.comp")
    .build();

app.addPostProcess(blurPass);
```

## Multithreading

Thread-safe entity reads. Entity mutation must happen on the main update loop.

```cpp
std::async(std::launch::async, [&] {
    auto view = app.scene().query<Mesh, Material>();
    for (auto [id, mesh, mat] : view) {
        // read-only access
    }
});
```

## Debug & Profiling

```cpp
app.setDebugLayer(true);                       // Validation layers
app.setGPUTracing(true);                       // Timestamp queries
app.onFrameEnd([](FrameDiagnostics& diag) {
    std::println("GPU: {:.2f} ms", diag.gpuTimeMs());
});
```

## Platform Notes

- **Windows**: CreateSurface using Win32 `HWND`.
- **Linux**: XCB or Wayland. Detect at compile time via `VKS_PLATFORM_XCB` or `VKS_PLATFORM_WAYLAND`.
- **macOS**: MoltenVK backend enabled automatically on Apple targets. No code changes required.

## FAQ

**Q: Do I need to know Vulkan to use this?**
A: No. The docs you are reading are all you need. Vulkan is an implementation detail.

**Q: Can I drop down to raw Vulkan?**
A: Yes. `app.vulkanDevice()` and `app.vulkanContext()` expose the underlying handles. Use with caution.

**Q: How does memory management work?**
A: Fully automatic. Assets are uploaded to GPU memory on load. No `VkDeviceMemory` or `VkBuffer` operations required.

**Q: What if I need a custom pipeline state?**
A: Use `Material::builder()` to declare any fixed-function state. If you need compute or raytracing, use the low-level escape hatch documented in the internals section.

## Contributing

See `CONTRIBUTING.md`. The internal renderer documentation lives in `docs/internals.md` for maintainers only.
