# Project Notes
This file is used by Junie to keep track of important project details, progress, and observations.

## Session History
- Initialized notes.md on 2026-02-20.
- Guidelines updated with README paths and notes.md usage.

### 2026-03-20 - Wireframe Shaders
- Added `WIREMESH_GLSL` to `ShaderDefinesEnum` and updated `ShaderDescriptor` to map it from GLSL.
- Updated `RenderPipelineManager` to set `VK_POLYGON_MODE_LINE` when `WIREMESH_GLSL` is detected.
- Created `res/shaders/glsl/entry/wiremesh.frag` and `res/shaders/glsl/entry/wiremesh_textured.frag`.
- Added `res/shaders/jsons/wiremesh.shader` and `res/shaders/jsons/wiremesh_textured.shader`.
- Updated `gfx::GraphicsEngine` and `vks::VulkanRenderer` to support an index in `setCameraData`.
- `DescriptorManager` now maintains a `std::vector<SceneUBO>` (scaled to 4 by default) instead of a single instance.
- `DescriptorManager::createSceneUBO` initializes multiple buffers and descriptor sets.
- `DescriptorManager::updateSceneUBO` takes a camera index to update the corresponding buffer.
- `RenderManager::recordCommandBuffer` now adds memory barriers for all active camera UBOs.
- Updated `RenderManager` to iterate through all active cameras and render the scene for each one.
- Each camera now has its own render pass targeting its specific offscreen framebuffer (via `pipelineManager->getFramebuffer(i, imageIndex)`).
- `RenderCommand` and `SkyboxRenderCommand` now include a `cameraIndex` to specify which camera pass they should be drawn in.
- `RenderSystem` and other callers now pass the correct `cameraIndex` (defaulting to 0) to `drawModel` and `drawSkybox`.
- `recordCommandBuffer` sorts the render queue by camera index and then by program ID to minimize pipeline switching.
- ImGui rendering is kept after the multi-camera render loop, as it typically renders to the main swapchain or a dedicated layer.
- Updated `VulkanRendererTests` and `RenderObjectTest` to match the new `GraphicsEngine` interface.

### 2026-04-04 - Vulkan Shadow Matrix Fix
- Updated shadow projection matrices in `RenderManager.cpp` to handle Vulkan's inverted Y-axis in clip space.
- Added `projection[1][1] *= -1.0f` to directional, point, and spot light shadow projections.
- Adjusted point light shadow `lookAt` up vectors to match Vulkan's cubemap coordinate system (inverted Y compared to OpenGL in world space, but effectively same as OpenGL's up vectors when combined with clip-space Y-flip).
- Directional light now uses `glm::ortho` followed by a Y-flip.
- Spot light now uses `glm::perspective` followed by a Y-flip.
- These changes ensure shadow maps are rendered with the correct orientation and depth range [0, 1] (via `GLM_FORCE_DEPTH_ZERO_TO_ONE`).

### Problem
Vulkan validation errors during shadow pipeline creation:
- `VK_SHADER_STAGE_GEOMETRY_BIT` uses push constants, but the pipeline layout didn't include this stage for push constant ranges.
- `pointLightSSBO` (Set 3, Binding 2) is used in the geometry shader (`cubeMap.geom`), but its descriptor set layout binding only specified `VK_SHADER_STAGE_FRAGMENT_BIT`.

### Changes
1. **DescriptorManager.cpp**: 
    - Updated `lightsLayout` (Set 3) bindings:
        - Binding 0 (`lightInfo`): Added `VK_SHADER_STAGE_VERTEX_BIT` (used in `shadowCubeMap.frag`).
        - Binding 1 (`directionalLightSSBO`): Added `VK_SHADER_STAGE_VERTEX_BIT`.
        - Binding 2 (`pointLightSSBO`): Added `VK_SHADER_STAGE_GEOMETRY_BIT` (for `cubeMap.geom`).
        - Binding 3 (`spotLightSSBO`): Added `VK_SHADER_STAGE_VERTEX_BIT`.
2. **RenderPipelineManager.cpp**:
    - Updated `createShadowPipeline`:
        - Included `VK_SHADER_STAGE_GEOMETRY_BIT` in the push constant range for `LIGHT_MODEL_PC_GLSL` to match `cubeMap.geom` usage.

### Verification
- Checked `cubeMap.geom`, `shadowCubeMap.vert`, and `shadowCubeMap.frag` to ensure all used resources have their stages correctly listed in layout definitions.
- The errors should now be resolved as the pipeline layout correctly reflects the shader stages using push constants and descriptor set resources.
