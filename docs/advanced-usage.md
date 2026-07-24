# Advanced Usage

## Custom Render Passes

Override a specific stage of the frame pipeline behind a custom render pass when post-processing is needed.

```cpp
RenderPass::Builder postFx()
    .withInput("hdrColor")
    .withDepthInput("sceneDepth")
    .withOutput("tonemapped", {.width = app.width(), .height = app.height()})
    .withFragment("shaders/tonemap.frag")
    .build();

app.addRenderPass(postFx, RenderPassStage::AfterScene);
```

## Compute Workloads

```cpp
ComputePass::Builder particles()
    .withInput("particles", ParticleBuffer::create(1'000'000))
    .withCompute("shaders/particle.comp", {.x = 256, .y = 1, .z = 1})
    .build();

app.addCompute(particles);
```

## Ray Tracing (Vulkan 1.3 + RT extension)

```cpp
if (app.hasExtension("VK_KHR_ray_tracing_pipeline")) {
    Material rtMat = Material::builder()
        .withRayTracingClosestHit("shaders/rt.closest")
        .withRayTracingMiss("shaders/rt.miss")
        .build();
}
```

## Custom Allocators

Override memory allocation strategy for a texture or buffer:

```cpp
Texture tex = Texture::load("huge_atlas.png", {
    .allocator = Allocator::preferVRAM()
});
```

## Headless Rendering

```cpp
App app({.headless = true, .width = 4096, .height = 4096});
// No window created. Render to an offscreen image.
Image renderResult = app.renderToImage([&](Frame& f) { ... });
renderResult.save("screenshot.exr");
```

## Hot-Reloading

```cpp
ShaderWatcher watcher;
watcher.watch("shaders/pbr.frag", [&](const Shader& newShader) {
    mat.replaceShader("frag", newShader);
});
// Watcher runs on a background thread and swaps shaders atomically each frame.
```

## Import Maps & Device Queues

```cpp
App::Config cfg = {
    .title = "Render Farm",
    .width = 0, .height = 0, // headless
    .requiredQueues = {
        QueueType::Graphics,
        QueueType::Compute,
        QueueType::Transfer,
        QueueType::Present
    },
    .preferredGPU = "NVIDIA",
    .debug = true
};

App app(cfg);
```
