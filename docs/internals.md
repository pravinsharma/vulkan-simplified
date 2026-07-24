# Internals

> This document is for maintainers only. End-user documentation does not expose these concepts.

## Architecture Overview

```
Application API Layer
    |
    v
Scene Graph + Material Graph
    |
    v
Command Encoding Layer (Frame objects)
    |
    v
Render Graph
    |
    v
VulkanBackend (swapchain, memory, queues, sync)
```

### Components

| Layer | Responsibility |
|-------|----------------|
| `vks::App` | Window, Vulkan instance, device enumeration, renderer lifecycle |
| `vks::Scene` | Entity storage, visibility culling, transform hierarchy |
| `vks::MaterialGraph` | Shader compilation, resource binding introspection |
| `vks::RenderGraph` | Automatic subpass layout, attachment aliasing, resource lifetime |
| `pimpl::VulkanBackend` | All raw Vulkan handles, command buffer pools, fences, semaphores |

## Render Pipeline

1. **Frame Begin** — Acquire swapchain image, reset command buffer, populate uniform buffers from `Camera`.
2. **Culling** — Frustum + occlusion culling on the scene query result.
3. **Render Graph Execution** — Ephemeral render passes managed by the graph; attachments are suballocated from a transient heap.
4. **Present** — Signal semaphore, queue present.

## Shader Compilation Pipeline

- Source watcher detects `.vert` / `.frag` changes.
- Parsed for `binding = N` uniforms and `location = N` vertex attributes.
- Compiles to SPIR-V via `glslang` or `dxc` (HLSL/MSL paths).
- Caches by hash in `~/.cache/vks/shaders/`.

## Memory Model

- **Static buffers** (vertex, index): staged via transfer queue, cached until hot-reload.
- **Dynamic uniform buffers**: per-frame, triple-buffered, sized at material compile time.
- **Textures**: GPU-local by default; fall back to coherent memory when `generateMips` is requested on unsupported formats.
- No manual `vkAllocateMemory`. Allocations pass through a pool allocator keyed by resource size class.

## Synchronization

All synchronization is encapsulated. Per-frame:
- Image available semaphore (swapchain)
- Render finished semaphore (next frame's wait)
- Fence for CPU-GPU flush

No `vkWaitForFences` exposed to the user.

## Error Handling

- `VK_ERROR_DEVICE_LOST` triggers a full swapchain recreation transparently.
- Validation errors during shader compilation are routed to the built-in logger.
- Fatal errors raise `vks::FatalError`; catch them at `main()` to display a dialog.

## Extending Internally

To add a new backend:

1. Implement `IRenderBackend` (`src/backend/irender_backend.hpp`).
2. Register in `App::Impl::selectBackend()`.
3. Provide a draw function for each `Topology` + `BlendMode` combination used by the material DSL.

## Build Configuration

| Flag | Effect |
|-----|--------|
| `VKS_ENABLE_VALIDATION` | Enables debug messenger |
| `VKS_ENABLE_GPU_ASSERTED` | Maps `debugReportEXT` to CRT assert |
| `VKS_MIN_VULKAN_VERSION` | Override minimum instance version (testing) |
| `VKS_HEADLESS` | Disables surface creation; outputs to image |
