# AGENTS.md

Rules and conventions for Kilo-assisted development in this repository.

## Project

- Name: `vulkan-simplified`
- Language: C++20
- Build system: CMake >= 3.24
- Graphics API target: Vulkan 1.3

## Commands

Run these from the repository root unless otherwise noted.

### Build
```
cmake --preship ciual -B build
cmake --build build --config Release
```

### Test
```
ctest --test-dir build -C Release
```

### Lint
```
cmake --build build --target vks_lint
```

### Format
```
cmake --build build --target vks_format
```

### Documentation
```
cmake --build build --target vks_docs
```

## Project structure

```
include/vulkan_simplified/   public headers (API surface)
src/                          implementation (renderer, backend, scene graph)
tests/                        unit and integration tests
shaders/                      runtime-loadable shader sources
docs/                         end-user and maintainer documentation
third_party/                  vendored dependencies (no git submodules unless required)
```

## Coding rules

- Follow the existing declarative style in the public API (`Material::builder()`, `Frame::draw(...)`).
- Do not introduce raw `Vk*` types in public headers.
- Keep `PIMPL` in `src/` and implementation headers out of `include/`.
- Add tests for every new public API or behavioral change.

## Documentation rules

- User-facing docs live in `docs/`. Do not add end-user content to `README.md` root unless it is a one-liner landing page.
- Maintainer internals belong in `docs/internals.md`.
- Keep examples minimal and self-contained.

## Git

- Use Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `test:`, `build:`, `chore:`).
- Do not push directly to `main`. Use feature branches and open PRs.
- Do not reformat unrelated code in a single commit. Split refactors into separate commits.

## Safety

- Never log secrets, keys, or GPU memory dumps.
- Do not silently swallow Vulkan errors; route them through the built-in logger.
