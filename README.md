# VulkanSimplified

A high-level C++20 abstraction over Vulkan 1.3. Developers describe **what** to draw; the library owns **how** it gets rendered.

No `VkDevice`, no `VkCommandBuffer`, no `VkPipeline`. Just `App`, `Scene`, `Material`, `Frame`.

```cpp
App app({.title = "My App", .width = 1280, .height = 720});

Mesh sphere = Mesh::load("sphere.obj");
Material mat = Material::builder()
    .withFragmentShader("shaders/pbr.frag")
    .withTexture("albedo", Texture::load("brick.png"))
    .withUniform("lightDir", Vec3{0.5f, -1.0f, 0.3f}.normalized())
    .build();

Entity entity = app.scene().create()
    .withMesh(sphere)
    .withMaterial(mat)
    .commit();

app.run([&](Frame& frame, float) {
    frame.draw(entity);
});
```

## Documentation

- [User Guide](docs/README.md)
- [API Reference](docs/api-reference.md)
- [Advanced Usage](docs/advanced-usage.md)
- [Roadmap](docs/ROADMAP.md)
- [Maintainer Internals](docs/internals.md)

## Build

```bash
cmake --preship ciual -B build
cmake --build build --config Release
```

## Requirements

- CMake >= 3.24
- C++20 compiler
- Vulkan SDK 1.3
- Windows / Linux / macOS

## License

[Add license]
