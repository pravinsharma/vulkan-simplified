# Examples

## hello_triangle.cpp

Minimal draw call. No textures, no camera movement.

```cpp
#include <vulkan_simplified/vulkan_simplified.hpp>
using namespace vks;

int main() {
    App app({.title = "Hello", .width = 800, .height = 600});

    Mesh tri = Mesh::fromVertices({
        {{-0.5f, -0.5f, 0}, { 1, 0, 0}},
        {{ 0.5f, -0.5f, 0}, { 0, 1, 0}},
        {{ 0.0f,  0.5f, 0}, { 0, 0, 1}},
    });

    auto mat = Material::builder()
        .withVertexShader("shaders/tri.vert")
        .withFragmentShader("shaders/tri.frag")
        .build();

    Entity e = app.scene().create()
        .withMesh(tri)
        .withMaterial(mat)
        .commit();

    app.run([&](Frame& f, float) { f.draw(e); });
}
```

`shaders/tri.vert`:

```glsl
#version 460
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;
void main() {
    gl_Position = vec4(inPosition, 1);
    outColor = inColor;
}
```

`shaders/tri.frag`:

```glsl
#version 460
layout(location = 0) in vec3 inColor;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(inColor, 1);
}
```

## textured_quad.cpp

Load a texture, rotate the quad over time.

```cpp
Material mat = Material::builder()
    .withVertexShader("shaders/quad.vert")
    .withFragmentShader("shaders/quad.frag")
    .withTexture("albedo", Texture::load("brick.png"))
    .build();

float angle = 0;
app.run([&](Frame& f, float dt) {
    angle += dt * 0.5f;
    scene.get(e).transform().rotation = Quat::fromEuler({0, Rad(angle), 0});
    f.draw(e);
});
```

## pbr_sphere.cpp

Physically-based rendering with environment map.

```cpp
Material pbr = Material::builder()
    .withVertexShader("shaders/pbr.vert")
    .withFragmentShader("shaders/pbr.frag")
    .withTexture("albedoMap",  Texture::load("gold_albedo.png"))
    .withTexture("normalMap",  Texture::load("gold_normal.png"))
    .withTexture("metallicMap",Texture::load("gold_metallic.png"))
    .withUniform("envIntensity", 1.5f)
    .build();
```

## compute_fade.cpp

Run a compute shader to animate particle positions.

```cpp
ComputePass fade = ComputePass::builder()
    .withInput("particles", particlesBuffer)
    .withCompute("shaders/fade.comp", {.x = 64, .y = 1, .z = 1})
    .build();

app.addCompute(fade);
```
