#pragma once

#include <memory>

#include "vks/types.hpp"

namespace vks {

struct CameraSettings {
    float fovDegrees = 60.0f;
    float aspectRatio = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 1000.0f;
};

class Camera {
public:
    explicit Camera(const CameraSettings& settings);

    Camera(const Camera& other);
    Camera& operator=(const Camera& other);
    Camera(Camera&&) noexcept;
    Camera& operator=(Camera&&) noexcept;

    ~Camera();

    void lookAt(Vec3 eye, Vec3 target, Vec3 up = Vec3{0.0f, 1.0f, 0.0f});

    Mat4 view() const;
    Mat4 projection() const;
    Mat4 viewProjection() const;

    Vec3 position() const;
    Vec3 forward() const;
    Vec3 up() const;
    Vec3 right() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
