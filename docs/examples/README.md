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

## pong.cpp

Self-playing Pong demo using two paddles and a ball. Demonstrates entities, transforms, `DrawCall` tint overrides, and manual per-frame updates.

```cpp
#include <vulkan_simplified/vulkan_simplified.hpp>
#include <algorithm>

using namespace vks;

static Mesh makeBox(float w, float h, float d) {
    float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
    return Mesh::fromVertices({
        {{-hw,-hh,-hd},{0,0,-1},{0,0}},{{ hw,-hh,-hd},{0,0,-1},{1,0}},
        {{ hw, hh,-hd},{0,0,-1},{1,1}},{{-hw, hh,-hd},{0,0,-1},{0,1}},
        {{-hw,-hh, hd},{0,0, 1},{0,0}},{{ hw,-hh, hd},{0,0, 1},{1,0}},
        {{ hw, hh, hd},{0,0, 1},{1,1}},{{-hw, hh, hd},{0,0, 1},{0,1}},
        {{-hw,-hd,-hh},{-1,0,0},{0,0}},{{-hw,-hd, hh},{-1,0,0},{1,0}},
        {{-hw, hd, hh},{-1,0,0},{1,1}},{{-hw, hd,-hh},{-1,0,0},{0,1}},
        {{ hw,-hd,-hh},{ 1,0,0},{0,0}},{{ hw,-hd, hh},{ 1,0,0},{1,0}},
        {{ hw, hd, hh},{ 1,0,0},{1,1}},{{ hw, hd,-hh},{ 1,0,0},{0,1}},
        {{-hd,-hh,-hw},{0,-1,0},{0,0}},{{ hd,-hh,-hw},{0,-1,0},{1,0}},
        {{ hd,-hh, hw},{0,-1,0},{1,1}},{{-hd,-hh, hw},{0,-1,0},{0,1}},
        {{-hd, hh,-hw},{0, 1,0},{0,0}},{{ hd, hh,-hw},{0, 1,0},{1,0}},
        {{ hd, hh, hw},{0, 1,0},{1,1}},{{-hd, hh, hw},{0, 1,0},{0,1}},
    });
}

int main() {
    App app({.title = "Pong", .width = 1280, .height = 720});

    Material solid = Material::builder()
        .withVertexShader("shaders/pong.vert")
        .withFragmentShader("shaders/pong.frag")
        .withCullMode(CullMode::Off)
        .withDepthTest(true)
        .withDepthWrite(true)
        .build();

    Mesh paddleMesh = makeBox(0.25f, 1.2f, 0.2f);
    Mesh ballMesh   = makeBox(0.3f, 0.3f, 0.3f);

    Scene& scene = app.scene();

    EntityId leftId  = scene.create().withMesh(paddleMesh).withMaterial(solid).commit();
    EntityId rightId = scene.create().withMesh(paddleMesh).withMaterial(solid).commit();
    EntityId ballId  = scene.create().withMesh(ballMesh).withMaterial(solid).commit();

    Camera cam = app.createCamera({.fovDegrees = 50, .aspectRatio = 1280.0f/720.0f, .near = 0.1f, .far = 100.0f});
    cam.lookAt({0,0,12}, {0,0,0}, {0,1,0});

    float ballVx = 5.0f * 0.6f;
    float ballVy = 5.0f * 0.8f;

    app.run([&](Frame& frame, float dt) {
        float halfH = 2.5f;
        float halfPaddle = 0.6f;

        scene.get(leftId).transform().position.y  = std::clamp(scene.get(ballId).transform().position.y, -halfH + halfPaddle, halfH - halfPaddle);
        scene.get(rightId).transform().position.y = std::clamp(scene.get(ballId).transform().position.y, -halfH + halfPaddle, halfH - halfPaddle);

        Vec3& bp = scene.get(ballId).transform().position;
        bp.x += ballVx * dt;
        bp.y += ballVy * dt;

        if (bp.y >= halfH - 0.15f || bp.y <= -halfH + 0.15f) ballVy = -ballVy;
        if (bp.x <= -3.5f && std::abs(bp.y - scene.get(leftId).transform().position.y) < halfPaddle + 0.15f) ballVx = std::abs(ballVx);
        if (bp.x >=  3.5f && std::abs(bp.y - scene.get(rightId).transform().position.y) < halfPaddle + 0.15f) ballVx = -std::abs(ballVx);
        if (bp.x < -5.0f || bp.x > 5.0f) { bp.x = 0; bp.y = 0; }

        frame.setCamera(cam);
        frame.clear(Color::fromHex(0x101018FF), ClearFlags::Color | ClearFlags::Depth);

        DrawCall l{leftId};  l.tintColor = Color::fromRGB(0.2f, 0.6f, 1.0f);
        DrawCall r{rightId}; r.tintColor = Color::fromRGB(1.0f, 0.3f, 0.3f);
        DrawCall b{ballId};  b.tintColor = Color::fromRGB(1.0f, 1.0f, 1.0f);
        frame.draw(l); frame.draw(r); frame.draw(b);
    });
}
```

`shaders/pong.vert`:

```glsl
#version 460 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(push_constant) struct Push {
    mat4 model;
    vec4 tintColor;
    float roughnessOverride;
    float metallicOverride;
} push;
layout(set = 0, binding = 0) uniform Camera { mat4 viewProjection; vec3 viewPosition; };
layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outTintColor;
layout(location = 4) out float outRoughnessOverride;
layout(location = 5) out float outMetallicOverride;
void main() {
    vec4 world = push.model * vec4(inPosition, 1.0);
    outWorldPos = world.xyz;
    outNormal = mat3(push.model) * inNormal;
    outUV = inUV;
    outTintColor = push.tintColor;
    outRoughnessOverride = push.roughnessOverride;
    outMetallicOverride = push.metallicOverride;
    gl_Position = viewProjection * world;
}
```

`shaders/pong.frag`:

```glsl
#version 460 core
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inTintColor;
layout(location = 4) in float inRoughnessOverride;
layout(location = 5) in float inMetallicOverride;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform Camera { mat4 viewProjection; vec3 viewPosition; };
void main() {
    vec3 n = normalize(inNormal);
    vec3 light = normalize(vec3(0.5, -1.0, 0.3));
    float diff = max(dot(n, -light), 0.0);
    outColor = vec4(inTintColor.rgb * (diff + 0.3), 1.0);
}
```

Build and run:

```bash
cmake --preship ciual -B build
cmake --build build --config Release
./build/Release/pong.exe
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
