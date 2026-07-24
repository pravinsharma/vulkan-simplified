#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "vks/camera.hpp"
#include "vks/frame.hpp"
#include "vks/scene_entity.hpp"

namespace vks {

class App {
public:
    struct Config {
        std::string title = "VulkanSimplified App";
        int width = 1280;
        int height = 720;
        bool vsync = true;
        bool headless = false;
    };

    explicit App(const Config& cfg);
    ~App();

    Scene& scene();
    Camera createCamera(const CameraSettings& settings);
    void run(const std::function<void(Frame&, float dt)>& loop);

    int width() const;
    int height() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

}
