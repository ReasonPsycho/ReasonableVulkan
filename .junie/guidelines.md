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

> **Note**: When building (e.g., `debug-with-tests`), it is highly recommended to use parallel building (e.g., `cmake --build --parallel` or `-j <number_of_cores>`) to significantly reduce build times.

### Assets
- The `res/` folder is symlinked to the build directory after building. Ensure all paths used in code are relative to this root.

## Documentation Workflow & README Guidelines
ReasonableVulkan maintains detailed, modular documentation across separate README files. Instead of relying on static summaries in these guidelines:
- **Read READMEs First**: Always read the relevant module and project README file(s) before starting work or making modifications in any part of the codebase.
- **Update READMEs After Changes**: If any changes, additions, refactorings, or architectural updates are made, you must update the corresponding README file(s) to keep documentation accurate and synchronized.

### Key Project READMEs
- `README.md`: Main project overview, build instructions, and prerequisites.
- `src/assetManager/README.md`: Comprehensive documentation for the Asset Manager module, asset types, caching, and serialization.
- `src/engine/README.md`: Documentation for the Engine module, ECS framework, scene graph, components, systems, and editor tooling.
- `src/vks/README.md`: Documentation for the Vulkan Support module, rendering pipelines, bindless descriptors, buffers, and synchronization.
- `res/shaders/README.md`: Shader creation, compilation pipeline, GLSL guidelines, and SPIR-V asset workflows.
- `res/shaders/hlsl/README.md`: Notes on HLSL shaders and status.
- `res/README.md`: Information regarding assets and sample asset packs.

## Coding Standards
- **Naming**: Follow existing patterns (e.g., camelCase for methods, PascalCase for classes).
- **Header Guards**: Use `#pragma once` or standard `#ifndef` guards as seen in the module.
- **Memory Management**: Prefer smart pointers (`std::shared_ptr`, `std::unique_ptr`) and automatic cleanup provided by modules like Asset Manager.
- **Namespaces**: Use module-specific namespaces like `am`, `vks`, `gfx`.

## Testing
- **Modular Test Suites**: Each module (`am`, `engine`, `vks`, `platform`, plus `integration_tests`) must maintain its own dedicated test suite.
- **Target Coverage**:
  - **Overall Project**: Aim for **70% – 80%** aggregate line coverage and **80% – 90%** non-trivial file coverage (`.cpp`).
  - **Core Logic & Asset Manager (`src/assetManager`, `src/engine/ecs`)**: Target **80% – 90%+** line coverage (deterministic algorithms, transforms, asset caching).
  - **Vulkan & Rendering (`src/vks`, `gfx`)**: Target **40% – 60%** line coverage (prioritize integration smoke tests and pipeline verification over mock-heavy driver unit tests).
  - **Tooling & UI (`fileBrowser`, UI panels)**: Target **30% – 50%** line coverage (test underlying data models and state management rather than layout callbacks).
- **Coverage Execution**:
  - Build with the `tests` preset (`cmake --preset tests` and `cmake --build --preset tests-parallel`) to compile with `--coverage`.
  - Run tests with CTest (`ctest --test-dir out/build/tests --output-on-failure`) or via CLion's **Run with Coverage** action.
  - Generate reports with `gcovr` (`python -m gcovr -r . --filter "src/" --html --html-details -o coverage.html`)

## Internal Tracking
- **Output Files**: Any command output should be redirected to `.junie/output.txt` to avoid cluttering files in the project root.