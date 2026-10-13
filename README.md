<img src="c66c4532-d051-4024-9ed0-e7098c933931.png" width="100" />

# ReasonableVulkan

My custom modular ECS game engine.

![EngineScreenshoot.png](showcase/EngineScreenshoot.png)

---
## Showcase

### Editor Functionality
- **Hierarchical Scene Graph**: Intuitive entity hierarchy with transform inheritance across nested objects.
![SceneGraph.gif](showcase/SceneGraph.gif)
- **Integrated File Browser**: Fully functional asset browser for managing textures, models, shaders, and project resources.
gif
- **Undo / Redo System**: Command-based history supporting `Ctrl+Z` / `Ctrl+Y` actions across entity and scene modifications.
gif
- **Multi-Scene & Prefabs**: Seamlessly switch between active scenes and instantiate reusable prefab templates.
gif
- **CLion Plugin Integration**: Direct IDE connection for synchronized workflows, project control, and development convenience.
gif
- **Scripting with Hot Reloading**: Rapid iteration via dynamic script reloading without restarting the engine.

### Optimization & Performance
gif
- **Bindless Vulkan Architecture**: High-throughput rendering with reduced CPU-GPU synchronization and flexible descriptor management.
- **High-Performance ECS**: Minimal-overhead Entity-Component-System architecture utilizing cache-friendly contiguous component storage.
gif
- **Fast Compilation Times**: Modular architecture and decoupled template implementations to minimize developer build friction.
gif
- **Custom Engine Asset Formats**: Streamlined binary formats and UUID-based caching for near-instant asset ingestion.

### Testing & Profiling
gif
- **Automated Test Suite**: Unit and integration test coverage for core systems, ECS lifecycle, and asset management.
gif
- **RenderDoc Integration**: Native support for in-depth Vulkan frame captures and graphics debugging.
gif
- **Tracy Profiler Integration**: Comprehensive real-time CPU/GPU performance tracing and frame budget analysis.

## Installation

---

### Prerequisites

Before you begin, make sure you have:

- **CMake ≥ 3.24** installed and added to your system PATH.
- **MinGW-w64** with `gcc`, `g++`, and `mingw32-make` in PATH. (other compilers may work but aren't actively used by me)
- **vcpkg** installed and the environment variable `VCPKG_ROOT` set to your vcpkg directory.
- **Vulkan SDK** installed and added to PATH

### Build Presets

This project uses CMake presets to simplify configuration. Presets are defined in `CMakePresets.json`.

Available presets:

- ``debug`` — Debug build without tests
- ``debug-with-tests`` — Debug build with unit and integration tests
- ``release`` — Optimized release build

### Notes

-----

- After build, a symbolic link to the ``res/`` folder is created in the build directory.
- Implicit Vulkan layers are disabled via `VK_LOADER_LAYERS_DISABLE=~implicit~` in CMake presets because they caused Vulkan validation layers to crash.
- There is plugin available for Clion here: https://github.com/ReasonPsycho/ReasonableVulkanPlugin