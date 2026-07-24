#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

#include "vulkan_simplified/camera.hpp"
#include "vulkan_simplified/material.hpp"
#include "vulkan_simplified/scene_entity.hpp"
#include "vulkan_simplified/types.hpp"

namespace vks {

namespace renderer {
class ForwardRenderer;
struct DrawRecord;
}

struct DrawCall {
    EntityId entity;
    Mat4 modelMatrix = Mat4(1.0f);
    std::optional<Color> tintColor = std::nullopt;
    std::optional<float> roughnessOverride = std::nullopt;
    std::optional<float> metallicOverride = std::nullopt;
};

class Frame {
public:
    Frame();
    ~Frame();

    void setRenderer(class renderer::ForwardRenderer* renderer);
    void setScene(class Scene& scene);
    void setCamera(const Camera& camera);
    void clear(Color color, int flags = ClearFlags::Color | ClearFlags::Depth);
    void draw(EntityId entity);
    void draw(std::span<const DrawCall> calls);
    void present();

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
