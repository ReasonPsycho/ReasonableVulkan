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
