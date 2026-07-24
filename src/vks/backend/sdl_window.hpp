#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <memory>
#include <string>

namespace vks::backend {

class SdlWindow {
public:
    struct CreateInfo {
        std::string title = "VulkanSimplified App";
        int width = 1280;
        int height = 720;
    };

    explicit SdlWindow(const CreateInfo& info);
    ~SdlWindow();

    SdlWindow(const SdlWindow&) = delete;
    SdlWindow& operator=(const SdlWindow&) = delete;
    SdlWindow(SdlWindow&&) noexcept;
    SdlWindow& operator=(SdlWindow&&) noexcept;

    bool pollEvent(SDL_Event& event);
    bool shouldClose() const;
    bool isMinimized() const;
    bool isIconified() const;
    int width() const;
    int height() const;
    SDL_Window* nativeHandle() const;

    struct Impl;
    Impl* getImpl() const { return impl_.get(); }

private:
    std::unique_ptr<Impl> impl_;
};

}
