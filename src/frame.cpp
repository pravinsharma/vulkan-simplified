#include "vks/frame.hpp"

#include "vks/backend/vulkan_context.hpp"
#include "vks/material.hpp"
#include "vks/scene_entity.hpp"
#include "vks/texture.hpp"
#include "vks/types.hpp"
#include "vks/renderer/renderer.hpp"

#include <glm/gtc/quaternion.hpp>

#include <stdexcept>

namespace vks {

static Mat4 toModelMatrix(const Transform& t) {
    Mat4 model = glm::translate(Mat4(1.0f), t.position);
    model *= glm::mat4_cast(t.rotation);
    model = glm::scale(model, t.scale);
    return model;
}

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
    pimpl->submitData.clearColor = color;
    pimpl->submitData.clearFlags = flags;
}

Frame::Frame() {
    pimpl = std::make_unique<Impl>();
}

void Frame::draw(EntityId entity) {
    if (!pimpl->submitData.scene) {
        pimpl->drawRecords.push_back({entity, Mat4(1.0f)});
        return;
    }
    try {
        const auto& ent = pimpl->submitData.scene->get(entity);
        pimpl->drawRecords.push_back({entity, toModelMatrix(ent.transform())});
    } catch (const std::exception&) {
        pimpl->drawRecords.push_back({entity, Mat4(1.0f)});
    }
}

void Frame::draw(std::span<const DrawCall> calls) {
    for (const auto& call : calls) {
        Mat4 model = Mat4(1.0f);
        if (pimpl->submitData.scene) {
            try {
                const auto& ent = pimpl->submitData.scene->get(call.entity);
                model = toModelMatrix(ent.transform());
            } catch (const std::exception&) {}
        }
        model = call.modelMatrix * model;

        vks::renderer::DrawRecord rec;
        rec.entity = call.entity;
        rec.modelMatrix = model;
        rec.tintColor = call.tintColor;
        rec.roughnessOverride = call.roughnessOverride;
        rec.metallicOverride = call.metallicOverride;
        pimpl->drawRecords.push_back(rec);
    }
}

void Frame::present() {
    if (!pimpl->renderer || !pimpl->cameraSet) return;
    pimpl->submitData.drawRecords = std::span<const vks::renderer::DrawRecord>(
        pimpl->drawRecords.data(), pimpl->drawRecords.size());
    pimpl->renderer->submitFrame(pimpl->submitData);
    pimpl->drawRecords.clear();
    pimpl->cameraSet = false;
}

}
