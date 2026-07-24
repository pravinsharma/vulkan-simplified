#include "vks/camera.hpp"

#include <cmath>
#include <stdexcept>

namespace vks {

struct Camera::Impl {
    CameraSettings settings;
    Vec3 eye = {0.0f, 0.0f, 0.0f};
    Vec3 target = {0.0f, 0.0f, -1.0f};
    Vec3 up = {0.0f, 1.0f, 0.0f};

    mutable Mat4 cachedView = Mat4(1.0f);
    mutable Mat4 cachedProjection = Mat4(1.0f);
    mutable bool viewDirty = true;
    mutable bool projDirty = true;
};

Camera::Camera(const Camera& other) : pimpl(std::make_unique<Impl>(*other.pimpl)) {}

Camera& Camera::operator=(const Camera& other) {
    if (this != &other) *pimpl = *other.pimpl;
    return *this;
}

Camera::Camera(Camera&&) noexcept = default;

Camera& Camera::operator=(Camera&&) noexcept = default;

Camera::~Camera() = default;

Camera::Camera(const CameraSettings& settings)
    : pimpl(std::make_unique<Impl>())
{
    pimpl->settings = settings;
}

void Camera::lookAt(Vec3 eye, Vec3 target, Vec3 up) {
    pimpl->eye = eye;
    pimpl->target = target;
    pimpl->up = up;
    pimpl->viewDirty = true;
}

Mat4 Camera::view() const {
    if (pimpl->viewDirty) {
        const Vec3 f = glm::normalize(pimpl->target - pimpl->eye);
        const Vec3 s = glm::normalize(glm::cross(f, pimpl->up));
        const Vec3 u = glm::cross(s, f);

        pimpl->cachedView = Mat4(1.0f);
        pimpl->cachedView[0][0] = s.x;  pimpl->cachedView[0][1] = u.x;  pimpl->cachedView[0][2] = -f.x;
        pimpl->cachedView[1][0] = s.y;  pimpl->cachedView[1][1] = u.y;  pimpl->cachedView[1][2] = -f.y;
        pimpl->cachedView[2][0] = s.z;  pimpl->cachedView[2][1] = u.z;  pimpl->cachedView[2][2] = -f.z;
        pimpl->cachedView[3][0] = -glm::dot(s, pimpl->eye);
        pimpl->cachedView[3][1] = -glm::dot(u, pimpl->eye);
        pimpl->cachedView[3][2] =  glm::dot(f, pimpl->eye);
        pimpl->viewDirty = false;
    }
    return pimpl->cachedView;
}

Mat4 Camera::projection() const {
    if (pimpl->projDirty) {
        const float fovRad = glm::radians(pimpl->settings.fovDegrees);
        const float f = 1.0f / std::tan(fovRad / 2.0f);
        const float rangeInv = 1.0f / (pimpl->settings.near - pimpl->settings.far);

        pimpl->cachedProjection = Mat4(0.0f);
        pimpl->cachedProjection[0][0] = f / pimpl->settings.aspectRatio;
        pimpl->cachedProjection[1][1] = f;
        pimpl->cachedProjection[2][2] = (pimpl->settings.near + pimpl->settings.far) * rangeInv;
        pimpl->cachedProjection[2][3] = -1.0f;
        pimpl->cachedProjection[3][2] = pimpl->settings.near * pimpl->settings.far * rangeInv * 2.0f;
        pimpl->projDirty = false;
    }
    return pimpl->cachedProjection;
}

Mat4 Camera::viewProjection() const {
    return projection() * view();
}

Vec3 Camera::position() const {
    return pimpl->eye;
}

Vec3 Camera::forward() const {
    return glm::normalize(pimpl->target - pimpl->eye);
}

Vec3 Camera::up() const {
    return pimpl->up;
}

Vec3 Camera::right() const {
    return glm::normalize(glm::cross(forward(), Vec3{0.0f, 1.0f, 0.0f}));
}

}
