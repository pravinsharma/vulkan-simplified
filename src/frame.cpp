#include "vulkan_simplified/frame.hpp"

#include "vulkan_simplified/backend/vulkan_context.hpp"
#include "vulkan_simplified/material.hpp"
#include "vulkan_simplified/scene_entity.hpp"
#include "vulkan_simplified/texture.hpp"
#include "vulkan_simplified/types.hpp"
#include "vulkan_simplified/renderer/renderer.hpp"

#include <stdexcept>

namespace vks {

struct Frame::Impl {
    vks::renderer::ForwardRenderer* renderer = nullptr;

    bool cameraSet = false;
    Vec4 clearColor = {0.05f, 0.05f, 0.1f, 1.0f};
    int clearFlags = static_cast<int>(vks::ClearFlags::Color) | static_cast<int>(vks::ClearFlags::Depth);

    vks::renderer::FrameSubmitData submitData;
    std::vector<vks::renderer::DrawRecord> drawRecords;
};

void Frame::setRenderer(class renderer::ForwardRenderer* renderer) {
    if (!renderer) return;
    pimpl->renderer = renderer;
}

void Frame::setScene(class Scene& scene) {
    pimpl->submitData.scene = &scene;
}

Frame::~Frame() = default;

void Frame::setCamera(const Camera& camera) {
    if (!pimpl->renderer) return;
    pimpl->submitData.viewProjection = camera.viewProjection();
    pimpl->submitData.viewPosition = camera.position();
    pimpl->cameraSet = true;
}

void Frame::clear(Color color, int flags) {
    pimpl->clearColor = color;
    pimpl->clearFlags = flags;
}

Frame::Frame() {
    pimpl = std::make_unique<Impl>();
}

void Frame::draw(EntityId entity) {
    pimpl->drawRecords.push_back({entity, Mat4(1.0f)});
}

void Frame::draw(std::span<const DrawCall> calls) {
    for (const auto& call : calls) {
        vks::renderer::DrawRecord rec;
        rec.entity = call.entity;
        rec.modelMatrix = call.modelMatrix;
        rec.tintColor = call.tintColor;
        rec.roughnessOverride = call.roughnessOverride;
        rec.metallicOverride = call.metallicOverride;
        pimpl->drawRecords.push_back(rec);
    }
}

void Frame::present() {
    if (!pimpl->renderer || !pimpl->cameraSet) return;
    pimpl->submitData.clearColor = pimpl->clearColor;
    pimpl->submitData.clearFlags = pimpl->clearFlags;
    pimpl->submitData.drawRecords = std::span<const vks::renderer::DrawRecord>(
        pimpl->drawRecords.data(), pimpl->drawRecords.size());
    pimpl->renderer->submitFrame(pimpl->submitData);
    pimpl->drawRecords.clear();
    pimpl->cameraSet = false;
}

}
