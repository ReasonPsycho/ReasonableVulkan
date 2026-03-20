# ReasonableVulkan Shader Guidelines

This directory contains GLSL shaders and shader program definitions used by the ReasonableVulkan engine. The engine uses a modular, include-based approach for shader authoring and automatically configures Vulkan pipelines based on `#define` markers in the source.

## Directory Structure
- `glsl/`: All GLSL source files.
  - `entry/`: Shader entry points (e.g., `mesh.vert`, `mesh.frag`). These are the files directly referenced in `.shader` JSONs.
  - `common/`: Shared structures like UBOs and Push Constants (e.g., `scene_ubo.glsl`, `model_pc.glsl`).
  - `vertex/`: Vertex attribute and IO definitions (`vertex_io.glsl`).
  - `material/`: Material-specific logic (e.g., `material_pbr.glsl`).
  - `lighting/`: Lighting models and light accumulation (e.g., `lighting_common.glsl`).
- `jsons/`: `.shader` files that group vertex and fragment stages into a single "Shader Program" for the Asset Manager.

## Shader Program Definitions (.shader)
To use a shader in the engine, define it in a JSON file within `res/shaders/jsons/`:
```json
{
  "vertex": "../glsl/entry/mesh.vert",
  "fragment": "../glsl/entry/mesh.frag"
}
```
The Asset Manager loads these stages, compiles them to SPIR-V, and tracks their requirements.

## GLSL Authoring Rules
1. **Version**: Use `#version 450`.
2. **Extensions**: Always enable `GL_ARB_shading_language_include` to support the engine's modular include system:
   ```glsl
   #extension GL_ARB_shading_language_include : enable
   ```
3. **Includes**: Use relative paths from the current file's directory.
4. **Modularity**: Place reusable logic (lighting, materials, descriptors) in the appropriate subdirectory and include it in your `entry/` shaders.

## Automated Pipeline Configuration (Define-to-VKS)
The engine uses a "Define-to-VKS" mechanism where the Asset Manager recursively scans shader source (including all nested `#include`s) for specific `#define` markers. These markers tell the Vulkan Support (`vks`) module which descriptors and pipeline features to enable.

### Standard System Defines
If any of these are defined in your shader (or its includes), the engine automatically configures the corresponding Vulkan resource:

| Define | Purpose | Effect in VKS |
|--------|---------|---------------|
| `SCENE_UBO_GLSL` | Global scene data (camera, time, etc.) | Adds Scene UBO Descriptor (Set 0) |
| `VERTEX_IO_GLSL` | Standard vertex input layout | Configures Vertex Input State |
| `MODEL_PC_GLSL` | Per-model transform push constant | Enables Push Constant range (mat4 model) |
| `MATERIAL_PBR_GLSL` | PBR material properties | Adds Material Texture/Parameter Descriptors |
| `LIGHTING_COMMON_GLSL` | Common lighting structures | Adds Lighting UBO/Storage Buffers |
| `MATERIAL_SKYBOX_GLSL` | Skybox-specific rendering | Configures Skybox-specific pipeline state |

### Example usage:
To use per-model transforms, simply include the common file which contains the define:
```glsl
// Inside mesh.vert
#include "../common/model_pc.glsl" // This file #defines MODEL_PC_GLSL
```
The engine will see `MODEL_PC_GLSL` and automatically add the `VkPushConstantRange` to the pipeline layout.

## Creating New Shaders
1. Create your entry points in `glsl/entry/`.
2. Reuse existing components from `glsl/common/`, `glsl/lighting/`, etc.
3. Ensure you use the standard `#define` guards in your include files.
4. Define a new `.shader` JSON in `jsons/`.
5. The Asset Manager will handle compilation and meta-data generation.