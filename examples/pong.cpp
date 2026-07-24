#include <vks/vks.hpp>
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include <cstdio>

using namespace vks;

static Mesh makeBox(float w, float h, float d) {
    float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
    std::array<Vertex, 24> verts{{
        {{-hw,-hh,-hd},{0,0,-1},{0,0}},{{ hw,-hh,-hd},{0,0,-1},{1,0}},
        {{ hw, hh,-hd},{0,0,-1},{1,1}},{{-hw, hh,-hd},{0,0,-1},{0,1}},
        {{-hw,-hh, hd},{0,0, 1},{0,0}},{{ hw,-hh, hd},{0,0, 1},{1,0}},
        {{ hw, hh, hd},{0,0, 1},{1,1}},{{-hw, hh, hd},{0,0, 1},{0,1}},
        {{-hw,-hd,-hh},{-1,0,0},{0,0}},{{-hw,-hd, hh},{-1,0,0},{1,0}},
        {{-hw, hd, hh},{-1,0,0},{1,1}},{{-hw, hd,-hh},{-1,0,0},{0,1}},
        {{ hw,-hd,-hh},{ 1,0,0},{0,0}},{{ hw,-hd, hh},{ 1,0,0},{1,0}},
        {{ hw, hd, hh},{ 1,0,0},{1,1}},{{ hw, hd,-hh},{ 1,0,0},{0,1}},
        {{-hd,-hh,-hw},{0,-1,0},{0,0}},{{ hd,-hh,-hw},{0,-1,0},{1,0}},
        {{ hd,-hh, hw},{0,-1,0},{1,1}},{{-hd,-hh, hw},{0,-1,0},{0,1}},
        {{-hd, hh,-hw},{0, 1,0},{0,0}},{{ hd, hh,-hw},{0, 1,0},{1,0}},
        {{ hd, hh, hw},{0, 1,0},{1,1}},{{-hd, hh, hw},{0, 1,0},{0,1}},
    }};
    return Mesh::fromVertices(std::span<const Vertex>(verts.data(), verts.size()));
}

struct Paddle {
    EntityId id;
    float x;
    float y;
    float w;
    float h;
    float speed;
    float targetY;
};

struct Ball {
    EntityId id;
    float x;
    float y;
    float z;
    float vx;
    float vy;
    float vz;
    float radius;
};

static constexpr float TABLE_W = 8.0f;
static constexpr float TABLE_H = 5.0f;
static constexpr float PADDLE_W = 0.25f;
static constexpr float PADDLE_H = 1.2f;
static constexpr float BALL_R = 0.15f;
static constexpr float PADDLE_SPEED = 4.0f;
static constexpr float BALL_SPEED = 5.0f;

int main() {
    try {
        App app({.title = "Pong", .width = 1280, .height = 720, .vsync = true});

        Material solid = Material::builder()
            .withVertexShader("pong.vert")
            .withFragmentShader("pong.frag")
            .withCullMode(CullMode::Off)
            .withDepthTest(true)
            .withDepthWrite(true)
            .build();

        Mesh paddleMesh = makeBox(PADDLE_W, PADDLE_H, 0.2f);
        Mesh ballMesh   = makeBox(BALL_R * 2.0f, BALL_R * 2.0f, BALL_R * 2.0f);

        Scene& scene = app.scene();

        Paddle left{
            scene.create()
                .withMesh(paddleMesh)
                .withMaterial(solid)
                .withTransform({.position = {-TABLE_W * 0.5f + 0.4f, 0.0f, 0.0f}})
                .commit(),
            -TABLE_W * 0.5f + 0.4f,
            0.0f,
            PADDLE_W,
            PADDLE_H,
            PADDLE_SPEED,
            0.0f
        };

        Paddle right{
            scene.create()
                .withMesh(paddleMesh)
                .withMaterial(solid)
                .withTransform({.position = {TABLE_W * 0.5f - 0.4f, 0.0f, 0.0f}})
                .commit(),
            TABLE_W * 0.5f - 0.4f,
            0.0f,
            PADDLE_W,
            PADDLE_H,
            PADDLE_SPEED,
            0.0f
        };

        Ball ball{
            scene.create()
                .withMesh(ballMesh)
                .withMaterial(solid)
                .withTransform({.position = {0.0f, 0.0f, 0.0f}})
                .commit(),
            0.0f,
            0.0f,
            0.0f,
            BALL_SPEED * 0.6f,
            BALL_SPEED * 0.8f,
            0.0f,
            BALL_R
        };

        Camera cam = app.createCamera({
            .fovDegrees = 50.0f,
            .aspectRatio = 1280.0f / 720.0f,
            .near = 0.1f,
            .far = 100.0f
        });
        cam.lookAt({0.0f, 0.0f, 12.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        app.run([&](Frame& frame, float dt) {
            const bool* keys = SDL_GetKeyboardState(nullptr);

            float halfTableH = TABLE_H * 0.5f;
            float halfPaddle = PADDLE_H * 0.5f;

            if (keys[SDL_SCANCODE_W]) {
                left.y = std::clamp(left.y + left.speed * dt, -halfTableH + halfPaddle, halfTableH - halfPaddle);
            }
            if (keys[SDL_SCANCODE_S]) {
                left.y = std::clamp(left.y - left.speed * dt, -halfTableH + halfPaddle, halfTableH - halfPaddle);
            }

            if (keys[SDL_SCANCODE_UP]) {
                right.y = std::clamp(right.y + right.speed * dt, -halfTableH + halfPaddle, halfTableH - halfPaddle);
            }
            if (keys[SDL_SCANCODE_DOWN]) {
                right.y = std::clamp(right.y - right.speed * dt, -halfTableH + halfPaddle, halfTableH - halfPaddle);
            }

            scene.get(left.id).transform().position.y = left.y;
            scene.get(right.id).transform().position.y = right.y;

            ball.x += ball.vx * dt;
            ball.y += ball.vy * dt;

            if (ball.y >= halfTableH - BALL_R || ball.y <= -halfTableH + BALL_R) {
                ball.vy = -ball.vy;
                ball.y = std::clamp(ball.y, -halfTableH + BALL_R, halfTableH - BALL_R);
            }

            bool hitLeft = ball.x <= left.x + PADDLE_W * 0.5f + BALL_R &&
                           ball.x >= left.x - 0.1f &&
                           std::abs(ball.y - left.y) < halfPaddle + BALL_R;
            bool hitRight = ball.x >= right.x - PADDLE_W * 0.5f - BALL_R &&
                            ball.x <= right.x + 0.1f &&
                            std::abs(ball.y - right.y) < halfPaddle + BALL_R;

            if (hitLeft) {
                ball.x = left.x + PADDLE_W * 0.5f + BALL_R;
                ball.vx = std::abs(ball.vx);
                ball.vy += (ball.y - left.y) * 2.0f;
            }
            if (hitRight) {
                ball.x = right.x - PADDLE_W * 0.5f - BALL_R;
                ball.vx = -std::abs(ball.vx);
                ball.vy += (ball.y - right.y) * 2.0f;
            }

            float speed = std::sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
            if (speed > BALL_SPEED * 1.5f) {
                ball.vx = (ball.vx / speed) * BALL_SPEED * 1.5f;
                ball.vy = (ball.vy / speed) * BALL_SPEED * 1.5f;
            }

            if (ball.x < -TABLE_W) {
                ball.x = 0.0f;
                ball.y = 0.0f;
                ball.vx = BALL_SPEED * 0.6f;
                ball.vy = BALL_SPEED * 0.8f;
            }
            if (ball.x > TABLE_W) {
                ball.x = 0.0f;
                ball.y = 0.0f;
                ball.vx = -BALL_SPEED * 0.6f;
                ball.vy = BALL_SPEED * 0.8f;
            }

            scene.get(ball.id).transform().position = {ball.x, ball.y, ball.z};

            frame.setCamera(cam);
            frame.clear(Color::fromHex(0x101018FF), ClearFlags::Color | ClearFlags::Depth);

            std::array<DrawCall, 3> calls;
            calls[0].entity = left.id;
            calls[0].tintColor = Color::fromRGB(0.2f, 0.6f, 1.0f);
            calls[1].entity = right.id;
            calls[1].tintColor = Color::fromRGB(1.0f, 0.3f, 0.3f);
            calls[2].entity = ball.id;
            calls[2].tintColor = Color::fromRGB(1.0f, 1.0f, 1.0f);
            frame.draw(calls);
        });

        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
}
