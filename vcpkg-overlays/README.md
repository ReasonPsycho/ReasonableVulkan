# vcpkg Overlays

This directory contains vcpkg overlay ports and/or triplets used by the ReasonableVulkan project.

## Overlay Ports
Overlay ports allow us to override existing vcpkg ports or add new ones without modifying the vcpkg repository itself.
They are enabled in the CMake configuration via the `VCPKG_OVERLAY_PORTS` variable.

## Usage
When using CMake presets (e.g., `debug`, `tests`), this directory is automatically included as an overlay ports location.

If you add a new port here, ensure it follows the vcpkg port structure:
`vcpkg-overlays/<port-name>/portfile.cmake`
`vcpkg-overlays/<port-name>/vcpkg.json`
