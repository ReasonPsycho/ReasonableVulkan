# Project Notes
This file is used by Junie to keep track of important project details, progress, and observations.

## Session History
- Initialized notes.md on 2026-02-20.
- Guidelines updated with README paths and notes.md usage.

### 2026-03-19 - Multi-Camera Support in VKS
- Updated `gfx::GraphicsEngine` and `vks::VulkanRenderer` to support an index in `setCameraData`.
- `DescriptorManager` now maintains a `std::vector<SceneUBO>` (scaled to 4 by default) instead of a single instance.
- `DescriptorManager::createSceneUBO` initializes multiple buffers and descriptor sets.
- `DescriptorManager::updateSceneUBO` takes a camera index to update the corresponding buffer.
- `RenderManager::recordCommandBuffer` now adds memory barriers for all active camera UBOs.
- `RenderManager` currently uses `sceneUBOs[0]` for both skybox and model rendering to maintain backward compatibility with single-camera rendering while providing the infrastructure for multiple cameras.
- Updated `RenderSystem` and `MockGraphicsEngine` to pass index 0.
