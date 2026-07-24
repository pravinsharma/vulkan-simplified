# Roadmap

## Goals

Make Vulkan usable for everyday game and graphics developers without exposing driver-level complexity.

## Milestones

### v0.1 — Prototype (complete)
- `App`, `Scene`, `Entity`, `Material`, `Camera`, `Frame`
- Basic forward renderer with triangle / mesh draw
- Hot-reloadable GLSL shaders
- OBJ mesh loading
- PNG texture loading
- Cross-platform window, input, and swapchain via SDL3
- CMake build with `VCPKG_ROOT` and `VULKAN_SDK` environment variables

### v0.2 — Stable API (complete)
- PBR material DSL with texture slots and uniforms (`PBRTextures`, `PBRUniforms`)
- Push constants and per-draw overrides (`DrawCall::tintColor`, `roughnessOverride`, `metallicOverride`)
- Depth, blend, cull, topology configuration via `MaterialBuilder`
- Multithreaded scene queries via `Scene::queryAsync<Components...>()`
- Debug layer and frame diagnostics via `DebugLayer` singleton

### v0.3 — Advanced Rendering (complete)
- Render graph with automatic subpass layout (`RenderGraph`, `RenderGraph::Pass`, `RenderGraphContext`)
- Post-processing pass framework (bloom, SSAO, tone-map via `BloomPass`, `SSAOPass`, `ToneMapPass`)
- Compute pass integration (post-process compute shader support)
- Headless / offscreen rendering for toolchains (`App::Config::headless`)

### v0.4 — Production Ready (in progress)
- MoltenVK backend for macOS
- Ray tracing pipeline (VK_KHR_ray_tracing_pipeline)
- Shader variants and permutation management
- Async asset streaming
- SPIR-V caching and shader precompilation

### v1.0 — Public Release
- Stable ABI, tagged release
- CMake package config and vcpkg manifest (via `VCPKG_ROOT`)
- Full CI on Windows / Linux / macOS
- User guide, API reference, and sample projects

## Example Roadmap (Build-Up)

### Phase 1 — Hello Triangle
- Single fullscreen quad with a solid color
- Verifies `App`, `Material`, `Frame` lifecycle

### Phase 2 — Moving Marker
- One mesh entity updated via model matrix each frame
- Verifies camera + transform updates

### Phase 3 — Interactive 2D Canvas (turtle graphics)
- Orthographic-style camera setup
- Command area below the canvas with on-screen text built from 3D quads
- Turtle state machine parsing: `forward 100`, `right 90`, `color 1 0.3 0`, `reset`
- Dynamic line-segment meshes drawn per command, preserving scene state across frames
- Implemented in `examples/turtle.cpp` (see `Turtle`, `execute()`, inline bitmap text callbacks)

### Phase 4 — UI Overlay
- Help panel toggled via Escape, rendered as scene entities  
- Blinking command cursor
- "Text handling" demonstrated by encoding characters as colored dot-matrix quads in world space, using the same material/draw system

## Backlog

- Editor/tooling integration (Unity / Unreal exporters)
- WebGPU fallback backend
- Multi-view / VR stereo rendering
- Networked entity replication hooks
- Memory budget and LOD system
