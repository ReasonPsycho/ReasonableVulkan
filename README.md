# ReasonableVulkan

My attempt at transitioning my game engine called ReasonableGL used during ZTGK onto vulkan and also splitting it into a
proper modular game engine structure aaaandd adding test.

Prerequisites
-------------

Before you begin, make sure you have:

- **CMake ≥ 3.24** installed and added to your system PATH.
- **MinGW-w64** with `gcc`, `g++`, and `mingw32-make` in PATH. (other compilers may work but aren't actively used by me)
- **vcpkg** installed and the environment variable `VCPKG_ROOT` set to your vcpkg directory.
- **Vulkan SDK** installed and added to PATH

Build Presets
-------------

This project uses CMake presets to simplify configuration. Presets are defined in `CMakePresets.json`.

Available presets:

- ``debug`` — Debug build without tests
- ``debug-with-tests`` — Debug build with unit and integration tests
- ``release`` — Optimized release build

Notes
-----

- After build, a symbolic link to the ``res/`` folder is created in the build directory.
