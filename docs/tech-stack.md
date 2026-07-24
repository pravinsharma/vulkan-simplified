# Tech Stack

## Language & Standard

- **C++20** — `CMAKE_CXX_STANDARD 20`, no extensions
- Compilers: MSVC 19.40+, GCC 13+, Clang 16+

## Graphics API

- **Vulkan 1.3** — minimum instance version; all public APIs hide raw `Vk*` handles
- **MoltenVK** — automatic backend on macOS; no user-facing code changes required

## Windowing & Platform

- **SDL3** — window creation, input, surface/swapchain presentation
- Platforms: Windows (Win32), Linux (XCB / Wayland), macOS (MoltenVK)

## Math

- **GLM** — vector, matrix, quaternion, and transform types exposed through `vks::` aliases

## Asset Loading

- **stb** — image decoding (PNG, HDR, etc.) via `Texture::load(...)`
- **tinyobjloader** (or equivalent) — OBJ mesh loading via `Mesh::load(...)`
- Formats: `RGBA8`, `R16G16B16A16SFloat`, BCn/DXT compressed textures

## Shader Compilation

- **Shaderc** (via `unofficial-shaderc` CMake package) — runtime GLSL/HLSL → SPIR-V
- Hot-reload: file watcher detects `.vert`, `.frag`, `.comp` changes and recompiles automatically
- Cached by content hash in `~/.cache/vks/shaders/`
- MSL path available through `dxc` fallback for macOS

## Build System

- **CMake >= 3.24**
- **vcpkg** — dependency management via `vcpkg.json`
- Environment variables: `VCPKG_ROOT`, `VULKAN_SDK`
- Install prefix: GNU standard layout (`lib/cmake/vks/`)

## Testing

- **Catch2** — unit and integration tests under `tests/`

## Logging & Debug

- Built-in logger routing validation errors, shader compilation diagnostics, and fatal errors
- Preprocessor toggles:
  - `VKS_ENABLE_VALIDATION` — enable Vulkan debug messenger
  - `VKS_ENABLE_GPU_ASSERTED` — map `debugReportEXT` to CRT assert
  - `VKS_MIN_VULKAN_VERSION` — override minimum instance version (testing)
  - `VKS_HEADLESS` — disable surface creation; render to image for toolchains

## Dependencies Summary

| Dependency | Purpose |
|-----------|---------|
| SDL3 | Windowing, input, swapchain surface |
| Vulkan Headers | Vulkan 1.3 API surface |
| GLM | Linear algebra |
| stb | Image I/O |
| Shaderc | Runtime shader compilation |
| Catch2 | Testing |
