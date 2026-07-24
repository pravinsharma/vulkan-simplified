#include "vks/app.hpp"

#include "vks/backend/vulkan_context.hpp"
#include "vks/backend/sdl_window.hpp"
#include "vks/camera.hpp"
#include "vks/frame.hpp"
#include "vks/renderer/renderer.hpp"

#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace vks {

struct App::Impl {
    Config cfg;
    std::unique_ptr<vks::backend::SdlWindow> window;
    std::unique_ptr<vks::backend::VulkanContext> ctx;
    std::unique_ptr<vks::renderer::ForwardRenderer> renderer;
    Scene scene;
    std::vector<Camera> cameras;
    bool running = false;
};

App::App(const Config& cfg) : pimpl(std::make_unique<Impl>()) {
    pimpl->cfg = cfg;

    vks::backend::SdlWindow::CreateInfo winInfo;
    winInfo.title = cfg.title;
    winInfo.width = cfg.width;
    winInfo.height = cfg.height;
    pimpl->window = std::make_unique<vks::backend::SdlWindow>(winInfo);

    pimpl->ctx = std::make_unique<vks::backend::VulkanContext>(
        *pimpl->window, cfg.headless, true);

    if (!pimpl->ctx->isValid()) {
        throw std::runtime_error("VulkanContext failed to initialize");
    }

    pimpl->renderer = std::make_unique<vks::renderer::ForwardRenderer>(pimpl->ctx.get());
    vks::renderer::ForwardRenderer::Config rcfg{};
    rcfg.vsync = cfg.vsync;
    rcfg.frameOverlap = 3;
    if (!pimpl->renderer->init(rcfg)) {
        throw std::runtime_error("ForwardRenderer failed to initialize");
    }
}

App::~App() = default;

Scene& App::scene() {
    return pimpl->scene;
}

Camera App::createCamera(const CameraSettings& settings) {
    Camera cam(settings);
    pimpl->cameras.push_back(cam);
    return pimpl->cameras.back();
}

int App::width() const {
    return pimpl->window->width();
}

int App::height() const {
    return pimpl->window->height();
}

void App::run(const std::function<void(Frame&, float dt)>& loop) {
    pimpl->running = true;
    uint32_t lastTicks = SDL_GetTicks();
    Frame frame;
    frame.setRenderer(pimpl->renderer.get());
    frame.setScene(pimpl->scene);

    while (pimpl->running) {
        SDL_Event event;
        while (pimpl->window->pollEvent(event)) {
            if (event.type == SDL_EVENT_QUIT) {
                pimpl->running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                pimpl->ctx->recreateSwapchain();
            }
        }

        uint32_t now = SDL_GetTicks();
        float dt = (now - lastTicks) / 1000.0f;
        lastTicks = now;

        frame.clear({0.05f, 0.05f, 0.1f, 1.0f});

        loop(frame, dt);

        frame.present();

        if (pimpl->window->isMinimized()) {
            SDL_Delay(16);
        }
    }
}

}
