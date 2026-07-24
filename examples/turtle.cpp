#include <vks/vks.hpp>
#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <cctype>

using namespace vks;

Mesh makeLineMeshXY(const Vec2& a, const Vec2& b, float w) {
    Vec2 dir = b - a;
    float len = std::hypot(dir.x, dir.y);
    if (len < 0.0001f) {
        return Mesh::fromVertices(std::span<const Vertex>{}, std::span<const uint32_t>{});
    }
    dir /= len;
    Vec2 n = {-dir.y, dir.x};
    float hw = w * 0.5f;
    Vec3 pa = {a.x + n.x*hw, a.y + n.y*hw, 0.0f};
    Vec3 pb = {a.x - n.x*hw, a.y - n.y*hw, 0.0f};
    Vec3 pc = {b.x - n.x*hw, b.y - n.y*hw, 0.0f};
    Vec3 pd = {b.x + n.x*hw, b.y + n.y*hw, 0.0f};
    std::array<Vertex, 4> v{{
        {pa, {0,0,1}, {0,0}},
        {pb, {0,0,1}, {1,0}},
        {pc, {0,0,1}, {1,1}},
        {pd, {0,0,1}, {0,1}},
    }};
    std::array<uint32_t, 6> idx = {0,1,2, 2,3,0};
    return Mesh::fromVertices(
        std::span<const Vertex>(v.data(), v.size()),
        std::span<const uint32_t>(idx.data(), idx.size()));
}

Mesh makeGrid(float xMin, float xMax, float yMin, float yMax, float step) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    for (float x = xMin; x <= xMax + 0.0001f; x += step) {
        verts.push_back({{x, yMin, -0.01f}, {0,0,1}, {0,0}});
        verts.push_back({{x, yMax, -0.01f}, {0,0,1}, {0,1}});
    }
    for (float y = yMin; y <= yMax + 0.0001f; y += step) {
        verts.push_back({{xMin, y, -0.01f}, {0,0,1}, {0,0}});
        verts.push_back({{xMax, y, -0.01f}, {0,0,1}, {1,0}});
    }
    for (size_t i = 0; i < verts.size(); i += 2) {
        idx.push_back(static_cast<uint32_t>(i));
        idx.push_back(static_cast<uint32_t>(i + 1));
    }
    return Mesh::fromVertices(
        std::span<const Vertex>(verts.data(), verts.size()),
        std::span<const uint32_t>(idx.data(), idx.size()));
}

Mesh makeRect(float x, float y, float w, float h, float z) {
    std::array<Vertex, 4> v{{
        {{x, y, z}, {0,0,1}, {0,0}},
        {{x+w, y, z}, {0,0,1}, {1,0}},
        {{x+w, y+h, z}, {0,0,1}, {1,1}},
        {{x, y+h, z}, {0,0,1}, {0,1}},
    }};
    std::array<uint32_t, 6> idx = {0,1,2, 2,3,0};
    return Mesh::fromVertices(
        std::span<const Vertex>(v.data(), v.size()),
        std::span<const uint32_t>(idx.data(), idx.size()));
}

Mesh makeTurtleMesh(float s) {
    std::array<Vertex, 3> v{{
        {{ 0.0f,  s,      0.0f}, {0,0,1}, {0.5f, 0.5f}},
        {{-s*0.7f, -s*0.6f, 0.0f}, {0,0,1}, {0.0f, 0.0f}},
        {{ s*0.7f, -s*0.6f, 0.0f}, {0,0,1}, {1.0f, 0.0f}},
    }};
    std::array<uint32_t, 3> idx = {0, 1, 2};
    return Mesh::fromVertices(
        std::span<const Vertex>(v.data(), v.size()),
        std::span<const uint32_t>(idx.data(), idx.size()));
}

struct Turtle {
    float x = 0.0f, y = 0.0f, angle = 90.0f;
    bool penDown = true;
    Color color = Color(1.0f, 0.3f, 0.15f, 1.0f);
    float lineWidth = 0.08f;
};

// 3x5 bitmap font encoded inline for each glyph
static const uint8_t FONT[46][5] = {
    {0x00,0x00,0x00,0x00,0x00},
    {0x02,0x05,0x07,0x05,0x05},
    {0x06,0x05,0x06,0x05,0x06},
    {0x06,0x04,0x04,0x04,0x06},
    {0x06,0x05,0x05,0x05,0x06},
    {0x07,0x04,0x06,0x04,0x07},
    {0x07,0x04,0x06,0x04,0x04},
    {0x06,0x04,0x05,0x05,0x06},
    {0x05,0x05,0x07,0x05,0x05},
    {0x07,0x02,0x02,0x02,0x07},
    {0x01,0x01,0x01,0x05,0x02},
    {0x04,0x05,0x06,0x05,0x04},
    {0x04,0x04,0x04,0x04,0x07},
    {0x05,0x07,0x07,0x05,0x05},
    {0x05,0x07,0x07,0x05,0x05},
    {0x02,0x05,0x05,0x05,0x02},
    {0x06,0x05,0x06,0x04,0x04},
    {0x02,0x05,0x05,0x05,0x02},
    {0x06,0x05,0x06,0x05,0x05},
    {0x06,0x04,0x06,0x01,0x06},
    {0x07,0x02,0x02,0x02,0x02},
    {0x05,0x05,0x05,0x05,0x02},
    {0x04,0x04,0x04,0x02,0x02},
    {0x05,0x05,0x07,0x07,0x02},
    {0x05,0x07,0x02,0x07,0x05},
    {0x04,0x04,0x02,0x02,0x02},
    {0x07,0x01,0x02,0x04,0x07},
    {0x02,0x05,0x05,0x05,0x02},
    {0x02,0x06,0x02,0x02,0x07},
    {0x06,0x04,0x02,0x04,0x07},
    {0x06,0x04,0x06,0x04,0x06},
    {0x04,0x05,0x07,0x01,0x01},
    {0x07,0x04,0x06,0x04,0x06},
    {0x06,0x04,0x06,0x05,0x06},
    {0x07,0x01,0x02,0x04,0x04},
    {0x02,0x06,0x02,0x06,0x02},
    {0x02,0x04,0x06,0x04,0x06},
    {0x00,0x00,0x00,0x02,0x02},
    {0x00,0x00,0x07,0x00,0x00},
    {0x00,0x00,0x07,0x07,0x00},
    {0x00,0x02,0x07,0x02,0x00},
    {0x01,0x02,0x04,0x00,0x00},
    {0x00,0x02,0x02,0x00,0x02},
    {0x06,0x04,0x04,0x04,0x06},
    {0x06,0x01,0x01,0x01,0x06},
    {0x00,0x00,0x00,0x00,0x07},
};

static int fontIndex(char c) {
    if (c == ' ') return 0;
    char up = std::toupper((unsigned char)c);
    if (up >= 'A' && up <= 'Z') return 1 + (up - 'A');
    if (up >= '0' && up <= '9') return 27 + (up - '0');
    switch (c) {
        case '.': return 37;
        case '-': return 38;
        case '=': return 39;
        case '+': return 40;
        case '/': return 41;
        case ':': return 42;
        case '(': return 43;
        case ')': return 44;
        case '_': return 45;
    }
    return 0;
}

void execute(const std::string& cmd, Turtle& t, Material& mat,
             Scene& scene, std::vector<Mesh>& meshes,
             std::vector<EntityId>& lines, std::vector<Color>& cols) {
    std::istringstream iss(cmd);
    std::string op;
    iss >> op;
    std::transform(op.begin(), op.end(), op.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    float v;
    if ((op == "forward" || op == "fd") && (iss >> v)) {
        float rad = glm::radians(t.angle);
        float nx = t.x + std::cos(rad) * v;
        float ny = t.y + std::sin(rad) * v;
        if (t.penDown && std::hypot(nx - t.x, ny - t.y) > 0.001f) {
            meshes.push_back(makeLineMeshXY({t.x, t.y}, {nx, ny}, t.lineWidth));
            Mesh& seg = meshes.back();
            lines.push_back(scene.create()
                                .withMesh(seg)
                                .withMaterial(mat)
                                .commit());
            cols.push_back(t.color);
        }
        t.x = nx; t.y = ny;
    } else if ((op == "back" || op == "bk") && (iss >> v)) {
        float rad = glm::radians(t.angle);
        float nx = t.x - std::cos(rad) * v;
        float ny = t.y - std::sin(rad) * v;
        if (t.penDown && std::hypot(nx - t.x, ny - t.y) > 0.001f) {
            meshes.push_back(makeLineMeshXY({t.x, t.y}, {nx, ny}, t.lineWidth));
            Mesh& seg = meshes.back();
            lines.push_back(scene.create()
                                .withMesh(seg)
                                .withMaterial(mat)
                                .commit());
            cols.push_back(t.color);
        }
        t.x = nx; t.y = ny;
    } else if ((op == "right" || op == "rt") && (iss >> v)) {
        t.angle -= v;
    } else if ((op == "left" || op == "lt") && (iss >> v)) {
        t.angle += v;
    } else if (op == "color") {
        float r, g, b;
        iss >> r >> g >> b;
        t.color = Color(r, g, b, 1.0f);
    } else if ((op == "width" || op == "w") && (iss >> v)) {
        t.lineWidth = std::max(0.01f, v);
    } else if (op == "penup" || op == "pu") {
        t.penDown = false;
    } else if (op == "pendown" || op == "pd") {
        t.penDown = true;
    } else if (op == "reset" || op == "clear" || op == "cls") {
        t = Turtle{};
        for (auto e : lines) scene.destroy(e);
        lines.clear();
        cols.clear();
    }
}

int main() {
    try {
        App app({.title = "Turtle Graphics", .width = 1280, .height = 720, .vsync = true});

        Material solid = Material::builder()
            .withVertexShader("turtle.vert")
            .withFragmentShader("turtle.frag")
            .withCullMode(CullMode::Off)
            .withDepthTest(true)
            .withDepthWrite(true)
            .build();

        Scene& scene = app.scene();

        Mesh bgMesh = makeRect(-8.0f, -3.0f, 16.0f, 9.0f, -0.05f);
        EntityId bgId = scene.create()
                             .withMesh(bgMesh)
                             .withMaterial(solid)
                             .commit();

        Mesh gridMesh = makeGrid(-8.0f, 8.0f, -2.0f, 6.0f, 1.0f);
        EntityId gridId = scene.create()
                               .withMesh(gridMesh)
                               .withMaterial(solid)
                               .commit();

        Mesh cpanelMesh = makeRect(-8.0f, -5.0f, 16.0f, 2.0f, -0.04f);
        EntityId cpanelId = scene.create()
                                  .withMesh(cpanelMesh)
                                  .withMaterial(solid)
                                  .commit();

        Turtle turtle;
        Mesh turtleMesh = makeTurtleMesh(0.18f);
        EntityId turtleId = scene.create()
                                  .withMesh(turtleMesh)
                                  .withMaterial(solid)
                                  .withTransform({.position = {turtle.x, turtle.y, 0.02f}})
                                  .commit();

        std::vector<EntityId> lineIds;
        std::vector<Color> lineCols;
        std::vector<Mesh> lineMeshes;
        std::string commandLine;
        std::vector<EntityId> textIds;
        std::vector<EntityId> helpIds;
        std::vector<Mesh> textMeshes;
        bool helpOn = false;
        bool cursor = true;
        float blinkT = 0.0f;

        Camera cam = app.createCamera({.fovDegrees = 45.0f,
                                       .aspectRatio = 1280.0f / 720.0f,
                                       .near = 0.1f, .far = 100.0f});
        cam.lookAt({0, 1.5f, 12.0f}, {0, 1.5f, 0}, {0, 1, 0});

        app.run([&](Frame& frame, float dt) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            static std::array<bool, 512> prevKeys{};
            std::array<bool, 512> cur{};
            for (int i = 0; i < 512; ++i) cur[i] = keys[i];

            for (int sc = SDL_SCANCODE_A; sc <= SDL_SCANCODE_Z; ++sc) {
                if (cur[sc] && !prevKeys[sc])
                    commandLine += (char)('a' + (sc - SDL_SCANCODE_A));
            }
            for (int sc = SDL_SCANCODE_0; sc <= SDL_SCANCODE_9; ++sc) {
                if (cur[sc] && !prevKeys[sc])
                    commandLine += (char)('0' + (sc - SDL_SCANCODE_0));
            }
            if (cur[SDL_SCANCODE_SPACE] && !prevKeys[SDL_SCANCODE_SPACE]) commandLine += ' ';
            if (cur[SDL_SCANCODE_PERIOD] && !prevKeys[SDL_SCANCODE_PERIOD]) commandLine += '.';
            if (cur[SDL_SCANCODE_MINUS] && !prevKeys[SDL_SCANCODE_MINUS]) commandLine += '-';
            if (cur[SDL_SCANCODE_BACKSPACE] && !prevKeys[SDL_SCANCODE_BACKSPACE]) {
                if (!commandLine.empty()) commandLine.pop_back();
            }
            if (cur[SDL_SCANCODE_RETURN] && !prevKeys[SDL_SCANCODE_RETURN]) {
                if (!commandLine.empty()) {
                    execute(commandLine, turtle, solid, scene, lineMeshes, lineIds, lineCols);
                    commandLine.clear();
                }
            }
            if (cur[SDL_SCANCODE_ESCAPE] && !prevKeys[SDL_SCANCODE_ESCAPE]) {
                helpOn = !helpOn;
                if (!helpOn) {
                    for (auto e : helpIds) scene.destroy(e);
                    helpIds.clear();
                }
            }

            prevKeys = cur;

            for (auto e : textIds) scene.destroy(e);
            textIds.clear();
            for (auto e : helpIds) scene.destroy(e);
            helpIds.clear();

            blinkT += dt;
            if (blinkT > 0.5f) { blinkT -= 0.5f; cursor = !cursor; }

            std::vector<DrawCall> draws;
            draws.push_back(DrawCall{.entity = bgId});
            draws.push_back(DrawCall{.entity = gridId});
            draws.push_back(DrawCall{.entity = cpanelId});
            for (size_t i = 0; i < lineIds.size(); ++i)
                draws.push_back(DrawCall{.entity = lineIds[i], .tintColor = lineCols[i]});
            draws.push_back(DrawCall{.entity = turtleId, .tintColor = Color(0.2f, 1.0f, 0.3f, 1.0f)});

            {
                std::vector<Vertex> verts;
                std::vector<uint32_t> idx;
                uint32_t base = 0;
                float x = -7.5f;
                float y = -4.2f;
                float cw = 0.11f, ch = 0.14f;
                float charW = cw * 3.0f + cw * 0.5f;
                std::string display = commandLine + (cursor ? "_" : " ");
                for (unsigned char c : display) {
                    int ci = fontIndex(c);
                    const uint8_t* glyph = FONT[ci];
                    for (int row = 0; row < 5; ++row) {
                        uint8_t bits = glyph[row];
                        for (int col = 0; col < 3; ++col) {
                            if (bits & (1u << (2u - (unsigned)col))) {
                                float px = x + (float)col * cw;
                                float py = y - (float)row * ch;
                                float d = cw * 0.18f;
                                float dh = ch * 0.18f;
                                verts.push_back({{px + d,      py - dh,      0.0f}, {0,0,1}, {0,0}});
                                verts.push_back({{px + cw - d, py - dh,      0.0f}, {0,0,1}, {1,0}});
                                verts.push_back({{px + d,      py - ch + dh, 0.0f}, {0,0,1}, {1,1}});
                                verts.push_back({{px + cw - d, py - ch + dh, 0.0f}, {0,0,1}, {0,1}});
                                idx.push_back(base + 0);
                                idx.push_back(base + 1);
                                idx.push_back(base + 2);
                                idx.push_back(base + 1);
                                idx.push_back(base + 3);
                                idx.push_back(base + 2);
                                base += 4;
                            }
                        }
                    }
                    x += charW;
                }
                if (!verts.empty()) {
                    textMeshes.push_back(Mesh::fromVertices(
                        std::span<const Vertex>(verts.data(), verts.size()),
                        std::span<const uint32_t>(idx.data(), idx.size())));
                    Mesh& txt = textMeshes.back();
                    EntityId tid = scene.create().withMesh(txt).withMaterial(solid).commit();
                    textIds.push_back(tid);
                    draws.push_back(DrawCall{.entity = tid, .tintColor = Color(0.3f, 1.0f, 0.3f, 1.0f)});
                }
            }

            if (helpOn && helpIds.empty()) {
                std::vector<Vertex> hverts;
                std::vector<uint32_t> hidx;
                uint32_t hbase = 0;
                float hx = -7.5f;
                float hy = 3.5f;
                float hcw = 0.11f, hch = 0.13f;
                float hcharW = hcw * 3.0f + hcw * 0.5f;
                const char* lines[] = {
                    "TURTLE COMMANDS  ( ESC to close )",
                    "FWD N        | BK N       - move fwd/back",
                    "RT DEG       | LT DEG     - turn right/left",
                    "COLOR R G B                - set pen color",
                    "WIDTH N                    - set line width",
                    "PENUP / PU  | PENDOWN / PD - lift/lower pen",
                    "RESET / CLR                - clear canvas",
                };
                for (const char* line : lines) {
                    for (unsigned char c : std::string(line)) {
                        int ci = fontIndex(c);
                        const uint8_t* glyph = FONT[ci];
                        for (int row = 0; row < 5; ++row) {
                            uint8_t bits = glyph[row];
                            for (int col = 0; col < 3; ++col) {
                                if (bits & (1u << (2u - (unsigned)col))) {
                                    float px = hx + (float)col * hcw;
                                    float py = hy - (float)row * hch;
                                    float d = hcw * 0.18f;
                                    float dh = hch * 0.18f;
                                    hverts.push_back({{px + d,      py - dh,      0.0f}, {0,0,1}, {0,0}});
                                    hverts.push_back({{px + hcw - d, py - dh,      0.0f}, {0,0,1}, {1,0}});
                                    hverts.push_back({{px + d,      py - hch + dh, 0.0f}, {0,0,1}, {1,1}});
                                    hverts.push_back({{px + hcw - d, py - hch + dh, 0.0f}, {0,0,1}, {0,1}});
                                    hidx.push_back(hbase + 0);
                                    hidx.push_back(hbase + 1);
                                    hidx.push_back(hbase + 2);
                                    hidx.push_back(hbase + 1);
                                    hidx.push_back(hbase + 3);
                                    hidx.push_back(hbase + 2);
                                    hbase += 4;
                                }
                            }
                        }
                        hx += hcharW;
                    }
                    hx = -7.5f;
                    hy -= 0.38f;
                }
                if (!hverts.empty()) {
                    textMeshes.push_back(Mesh::fromVertices(
                        std::span<const Vertex>(hverts.data(), hverts.size()),
                        std::span<const uint32_t>(hidx.data(), hidx.size())));
                    Mesh& helpMesh = textMeshes.back();
                    EntityId hid = scene.create().withMesh(helpMesh).withMaterial(solid).commit();
                    helpIds.push_back(hid);
                    draws.push_back(DrawCall{.entity = hid, .tintColor = Color(0.9f, 0.85f, 0.5f, 1.0f)});
                }
            }

            scene.get(turtleId).transform().position = {turtle.x, turtle.y, 0.02f};
            scene.get(turtleId).transform().rotation =
                glm::angleAxis(glm::radians(turtle.angle - 90.0f), Vec3(0, 0, 1));

            frame.setCamera(cam);
            frame.clear(Color(0.05f, 0.05f, 0.08f, 1.0f), ClearFlags::Color | ClearFlags::Depth);
            frame.draw(draws);
            frame.present();
        });

        return 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
}
