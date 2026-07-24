#include "vks/backend/sdl_window.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <atomic>
#include <stdexcept>
#include <string>

namespace vks::backend {

struct SdlWindow::Impl {
    SDL_Window* window = nullptr;
    std::atomic<bool> close_requested{false};
};

SdlWindow::SdlWindow(const CreateInfo& info) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error("SDL_Init failed: " + std::string(SDL_GetError()));
    }

    impl_ = std::make_unique<Impl>();
    Uint32 sdl_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    impl_->window = SDL_CreateWindow(
        info.title.c_str(),
        static_cast<Uint32>(info.width),
        static_cast<Uint32>(info.height),
        sdl_flags
    );

    if (!impl_->window) {
        throw std::runtime_error("SDL_CreateWindow failed: " + std::string(SDL_GetError()));
    }
}

SdlWindow::~SdlWindow() {
    if (impl_ && impl_->window) {
        SDL_DestroyWindow(impl_->window);
        impl_->window = nullptr;
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

SdlWindow::SdlWindow(SdlWindow&& other) noexcept : impl_(std::move(other.impl_)) {
    other.impl_ = nullptr;
}

SdlWindow& SdlWindow::operator=(SdlWindow&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
        other.impl_ = nullptr;
    }
    return *this;
}

bool SdlWindow::pollEvent(SDL_Event& event) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            impl_->close_requested.store(true, std::memory_order_relaxed);
        }
        return true;
    }
    return false;
}

bool SdlWindow::shouldClose() const {
    return impl_->close_requested.load(std::memory_order_relaxed);
}

bool SdlWindow::isMinimized() const {
    return (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MINIMIZED) != 0;
}

bool SdlWindow::isIconified() const {
    return (SDL_GetWindowFlags(impl_->window) & SDL_WINDOW_MINIMIZED) != 0;
}

int SdlWindow::width() const {
    int w = 0;
    SDL_GetWindowSize(impl_->window, &w, nullptr);
    return w;
}

int SdlWindow::height() const {
    int h = 0;
    SDL_GetWindowSize(impl_->window, nullptr, &h);
    return h;
}

SDL_Window* SdlWindow::nativeHandle() const {
    return impl_ ? impl_->window : nullptr;
}

}
