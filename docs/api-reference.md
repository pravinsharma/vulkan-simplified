# API Reference

## App

```cpp
class App {
public:
    struct Config { std::string title; int width; int height; bool vsync = true; };
    explicit App(const Config& cfg);

    Scene& scene();
    Camera createCamera(const CameraSettings&);
    void run(const std::function<void(Frame&, float dt)>& loop);

    // low-level escape hatches (advanced users only)
    VulkanDevice& vulkanDevice();   // NOLINT
    VulkanContext& vulkanContext(); // NOLINT

    int width() const;
    int height() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};
```

## Scene & Entity

```cpp
class Scene {
public:
    EntityBuilder create();
    Entity& get(EntityId id);
    void destroy(EntityId id);
    QueryView<Mesh, Material, Transform> query();  // read-only iteration

private:
    struct Impl;
};
```

```cpp
class EntityBuilder {
public:
    EntityBuilder& withMesh(const Mesh&);
    EntityBuilder& withMaterial(const Material&);
    EntityBuilder& withTransform(const Transform&);
    EntityId commit();
};
```

Transform:
```cpp
struct Transform {
    Vec3  position   = {0, 0, 0};
    Quat  rotation   = Quat::identity();
    Vec3  scale      = {1, 1, 1};
};
```

## Material

```cpp
class Material {
public:
    struct Builder {
        Builder& withVertexShader(std::string path);
        Builder& withFragmentShader(std::string path);
        Builder& withComputeShader(std::string path);
        Builder& withTexture(std::string name, const Texture&);
        Builder& withUniform(std::string name, T value);
        Builder& withPushConstant(std::string name, T value);
        Builder& withBlendMode(BlendMode);
        Builder& withDepthTest(bool);
        Builder& withDepthWrite(bool);
        Builder& withCullMode(CullMode);
        Builder& withTopology(Topology);
        Material build();
    };
};
```

## Camera

```cpp
struct CameraSettings {
    float    fovDegrees;
    float    aspectRatio;
    float    near;
    float    far;
};
class Camera {
public:
    void lookAt(Vec3 eye, Vec3 target, Vec3 up = {0,1,0});
    Mat4 view() const;
    Mat4 projection() const;
    Mat4 viewProjection() const;
};
```

## Frame

```cpp
class Frame {
public:
    void setCamera(const Camera&);
    void clear(Color rgba, ClearFlags);
    void draw(EntityId id);
    void draw(std::span<const EntityId> ids);

    // draw with per-draw override
    void draw(EntityId id, const DrawCall& call);
};
```

## Texture

```cpp
class Texture {
public:
    static Texture load(std::string path, TextureDesc desc = {});
    int width() const;
    int height() const;
    TextureFormat format() const;
    bool hasMips() const;
};
```

## Mesh

```cpp
class Mesh {
public:
    static Mesh load(std::string path);
    uint32_t vertexCount() const;
    uint32_t indexCount() const;
    BoundingBox bounds() const;
};
```

## Types

```cpp
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat4 = glm::mat4;
using Quat = glm::quat;

struct Color : Vec4 { /* helper ctors from rgb(), hex() */ };

enum class BlendMode { Off, Alpha, Additive };
enum class CullMode  { Off, Front, Back };
enum class Topology  { TriangleList, TriangleStrip, LineList, PointList };
enum class TextureFormat { RGBA8, R16G16B16A16SFloat, BC1, BC3 };

struct ClearFlags { enum { Color = 1, Depth = 2, Stencil = 4 }; using Type = int; };
```
