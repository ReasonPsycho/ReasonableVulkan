# ReasonableVulkan Project Guidelines

## Overview
ReasonableVulkan is a modular game engine transitioned from ReasonableGL to Vulkan. It emphasizes a structured approach with Entity-Component-System (ECS) architecture, centralized asset management, and comprehensive testing.

## Prerequisites
- **CMake ≥ 3.24**
- **MinGW-w64** (gcc, g++, mingw32-make)
- **vcpkg** (Environment variable `VCPKG_ROOT` must be set)
- **Vulkan SDK**

## Build & Run
### Build Presets
- `debug`: Standard debug build.
- `debug-with-tests`: Includes unit and integration tests.
- `release`: Optimized release build.

### Assets
- The `res/` folder is symlinked to the build directory after building. Ensure all paths used in code are relative to this root.

## Module Guidelines

### Asset Manager (`am`)
The Asset Manager provides centralized, UUID-based asset loading and caching.
- **Singleton**: Access via `am::AssetManager::getInstance()`.
- **UUID Identification**: All assets are uniquely identified using Boost UUIDs.
- **Lazy Loading**: Assets are loaded on-demand when requested.
- **Supported Formats**:
  - Models: `.fbx`
  - Textures: `.png`
  - Shaders: `.spdv`, `.spv`, `.frag`, `.vert`
- **Factories**: Use the factory pattern to extend support for new asset types.

### Engine Module
The core orchestrator using ECS and scene management.
- **ECS**: Use the Entity-Component-System for all game logic.
- **Scenes**: Supports multiple scenes with hierarchical scene graphs (transform inheritance).
- **Transform System**: Built-in 3D transformation handling with matrix calculations.
- **Singleton**: Access via `Engine::getInstance()`.

### Vulkan Support (`vks`)
Vulkan-specific helpers and managers.
- **VulkanContext**: Encapsulates instance, device, and queue management.
- **RenderManager**: Handles frame submission, command buffer recording, and synchronization (Double buffering by default, `MAX_FRAMES_IN_FLIGHT = 2`).
- **RenderPipelineManager**: Manages Vulkan pipelines and shaders.
- **DescriptorManager**: Handles descriptor sets and uniform buffers.

## Shader Guidelines
### GLSL
The engine currently supports GLSL using `glslang` for compilation.
- **Compilation**: Shaders are compiled to SPIR-V using the `glslang` library or external tools during asset loading.
- **Supported extensions**: `.vert`, `.frag`, `.spv`, `.spdv`.

## Coding Standards
- **Naming**: Follow existing patterns (e.g., camelCase for methods, PascalCase for classes).
- **Header Guards**: Use `#pragma once` or standard `#ifndef` guards as seen in the module.
- **Memory Management**: Prefer smart pointers (`std::shared_ptr`, `std::unique_ptr`) and automatic cleanup provided by modules like Asset Manager.
- **Namespaces**: Use module-specific namespaces like `am`, `vks`, `gfx`.

## Testing
- For the time being, don't run any tests unless the user requests.

## Documentation & Notes
### Project READMEs
The following README files provide detailed documentation for different parts of the project:
- `README.md`: Main project overview, build instructions, and prerequisites.
- `res/README.md`: Information about the Vulkan samples asset pack.
- `res/shaders/hlsl/README.md`: Notes on HLSL shaders and their status.
- `src/assetManager/README.md`: Detailed documentation for the Asset Manager module.
- `src/engine/README.md`: Overview of the Engine module and its ECS architecture.
- `src/vks/README.md`: Documentation for the Vulkan Support module (currently empty).

### Internal Tracking
- **Notes File**: Use `.junie/notes.md` to keep track of progress, technical decisions, and observations throughout the development process. This file serves as a persistent scratchpad for Junie.
- **Output Files**: Any command output should be redirected to `.junie/output.txt` to avoid cluttering files in the project root.