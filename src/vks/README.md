# Vulkan Support Module (`vks`)

## Overview

The Vulkan Support (`vks`) module provides low-level and high-level Vulkan rendering abstractions, pipeline configurations, descriptor handling, and frame synchronization for the ReasonableVulkan engine. It implements the `gfx::GraphicsEngine` interface to provide hardware-accelerated rendering passes including PBR forward lighting, multi-light shadow mapping (directional, point, and spot lights), skybox rendering, and HUD/UI overlays.

## Features

- **Vulkan Abstraction Layer**: Encapsulates instance, logical device, queue families, command pools, and memory allocation.
- **Render Management**: Orchestrates frame synchronization (double buffering with `MAX_FRAMES_IN_FLIGHT = 2`), draw command queues, and pass execution.
- **Pipeline & Shader Management**: Configures graphics pipelines, shadow map passes, offscreen camera targets, and pipeline layouts.
- **Bindless & Unified Descriptors**: High-capacity bindless descriptor sets for textures and materials, alongside SSBOs for instances, lights, and materials.
- **Multi-Camera & Offscreen Rendering**: Supports multiple active cameras, offscreen render targets, and asset thumbnail generation.
- **Comprehensive Shadow Mapping**: Dedicated render passes and framebuffer arrays for directional, spot, and omnidirectional point light shadows.
- **Swapchain & Presentation**: Window surface handling, swapchain recreation, image presentation, and resize management.
- **ImGui Integration**: Integrated debug UI and overlay rendering support.

## Architecture & Class Reference

### 1. Core Engine & Context

#### `VulkanRenderer`
The top-level class implementing the `gfx::GraphicsEngine` interface:
- **Frame Lifecycle**: Coordinates `beginFrame()`, `renderFrame()`, and `endFrame()`.
- **Resource Management**: Loads models, meshes, shaders, materials, and textures into GPU descriptors, maintaining UUID-to-handle mappings.
- **Draw Call Dispatch**: Translates high-level engine draw commands (`drawModel`, `drawSkybox`, `drawLight`, `submitHUDItems`) into queued rendering commands.
- **Camera & Viewport Control**: Configures active camera matrices (projection, view, position) and exposes viewport texture pointers for editor viewports.
- **Offscreen & Thumbnails**: Captures offscreen camera images and generates cached thumbnail textures for project asset browsers.

#### `VulkanContext`
Encapsulates fundamental Vulkan hardware, instance, and device abstractions:
- **Instance & Device Management**: Initializes `VkInstance`, `VkPhysicalDevice`, and `VkDevice` with validation layers and required device extensions.
- **Queue Families & Command Pools**: Manages dedicated Graphics and Transfer queues along with corresponding command pools.
- **Memory Allocation & Buffers**: Creates `VkBuffer` and allocates `VkDeviceMemory` based on memory property flags, providing single-time command execution helpers for memory staging and copying.

#### `SwapChainManager`
Manages display presentation, surface configuration, and swapchain lifecycle:
- **Surface & Swapchain Creation**: Creates `VkSurfaceKHR` and configures `VkSwapchainKHR` tailored to window dimensions and surface capabilities.
- **Image Lifecycle**: Manages color image views, swapchain format selection, and present modes (FIFO, Mailbox).
- **Presentation Synchronization**: Handles acquiring next image indices (`acquireNextImage`) and presenting rendered frames to the graphics queue (`queuePresent`).
- **Resize Handling**: Recreates swapchains and image views seamlessly during window resizing or minimization.

### 2. Render Orchestration & Pipelines

#### `RenderManager`
Coordinates draw commands, frame synchronization, and Vulkan command buffer recording:
- **Frame Synchronization**: Implements double buffering (`MAX_FRAMES_IN_FLIGHT = 2`) using per-frame `VkSemaphore` (image available, render finished) and `VkFence` (in-flight sync) primitives.
- **Command Queuing**: Batches draw commands for geometry (`RenderCommand`), skyboxes (`SkyboxRenderCommand`, `SkyboxClearCommand`), lights (`DirectionalLightBufferData`, `PointLightBufferData`, `SpotLightBufferData`), and 2D HUD overlays (`HUDDrawItem`).
- **Render Pass Execution**: Records command buffers across shadow mapping passes (directional, spot, and omnidirectional point lights), main color/depth camera passes, and UI overlay passes.
- **Buffer & Uniform Updates**: Updates per-frame uniform buffers, instance SSBOs, and light storage buffers prior to submission.

#### `RenderPipelineManager`
Creates and manages Vulkan graphics pipelines, render passes, layouts, and offscreen framebuffers:
- **Pipeline Construction**: Compiles and caches `VkPipeline` and `VkPipelineLayout` objects for PBR forward shading, skyboxes, shadow depth passes, and HUD shaders.
- **Render Pass Creation**: Configures multi-attachment render passes for standard color/depth rendering as well as multiview shadow render passes.
- **Offscreen Framebuffer Management**: Maintains per-camera color and depth render targets (`CameraResources`) enabling multi-viewport and editor rendering.
- **Shadow Map Resources**: Allocates depth framebuffer arrays and image views for up to 4 directional, 4 point (cubemap multiview), and 4 spot light shadow maps.

#### `ImguiManager`
Manages Dear ImGui integration for editor and debug overlays:
- **Vulkan Backend Integration**: Initializes ImGui with Vulkan descriptor pools, render passes, and pipeline state.
- **Lifecycle Coordination**: Handles `beginFrame()`, UI command dispatch, and command buffer recording during rendering passes.
- **Texture Binding**: Exposes helpers to register Vulkan texture descriptors for rendering inside ImGui panels.

### 3. Descriptors & Resource Management

#### `DescriptorManager`
Central descriptor set and GPU resource registry:
- **Bindless Descriptor Architecture**: Allocates and updates high-capacity bindless descriptor sets for textures (`MAX_BINDLESS_TEXTURES = 4096`) and materials (`MAX_MATERIALS = 4096`).
- **Layout Management**: Maintains descriptor set layouts for scene uniforms, materials, meshes, lights, and bindless textures.
- **Resource Pooling**: Contains typed `ResourcePool` instances for models, meshes, shaders, materials, and textures.
- **Slot Allocation**: Manages sequential registration slots for textures and materials, mapping UUIDs to generational handles.

#### `ResourcePool<T, Tag>`
A generic, type-safe, generational resource container:
- **Handle Safety**: Allocates and resolves `Handle<Tag>` handles containing index and generation counters to prevent use-after-free errors.
- **Slot Recycling**: Reclaims and reuses freed slots efficiently in O(1) time.
- **Direct Access**: Provides fast O(1) pointer dereferencing for active descriptors.

#### Resource Descriptors (`IVulkanDescriptor` Hierarchy)
- **`IVulkanDescriptor`**: Abstract base class representing any GPU-backed Vulkan resource descriptor.
- **`ModelDescriptor`**: Represents a loaded 3D model containing a hierarchy of nodes (`NodeDescriptorStruct`), mesh handles, material handles, and transform data.
- **`MeshDescriptor`**: Encapsulates vertex and index `VkBuffer` allocations, vertex counts, submesh ranges, and bounding box dimensions.
- **`MaterialDescriptor`**: Manages PBR material parameters (albedo, roughness, metallic, emissive factors), texture slot references, and GPU material SSBO synchronization.
- **`TextureDescriptor`**: Wraps `VkImage`, `VkImageView`, `VkDeviceMemory`, and `VkSampler` for 2D and cubemap textures along with mipmap metadata.
- **`ShaderDescriptor`**: Represents an individual compiled SPIR-V shader stage (vertex, fragment, compute).
- **`ShaderProgramDescriptor`**: Pairs vertex and fragment shader stages into a complete pipeline definition, capturing reflection data and define requirements.

### 4. GPU Storage Buffers & Push Constants

#### Buffer Classes (`src/vks/src/descriptorManager/buffers/`)
- **`MaterialSSBO`**: GPU Shader Storage Buffer Object storing contiguous arrays of `GPUMaterialData` for direct indexing in bindless shaders.
- **`InstanceSSBO`**: GPU SSBO holding transformation matrices (`mat4 model`, `mat4 normalMatrix`), material indices, and instance flags for batched instanced rendering.
- **`LightSSBO`**: GPU SSBO containing directional, point, and spot light parameters (colors, positions, directions, intensities, cutoff angles) and light-space shadow view-projection matrices.
- **`SceneUBO`**: Uniform Buffer Object containing camera view, projection, inverse matrices, camera world position, ambient lighting, and global frame time.
- **`ShadowMapArray`**: Manages offscreen depth textures, samplers, and multi-layer image views used for shadow map lookups in lighting shaders.

#### Push Constants
- **`ModelPushConstant`**: Per-draw transform and instance index push constant payload.
- **`LightModelPushConstant`**: Model transform and light index payload for shadow map render passes.
- **`HUDPushConstant`**: Screen-space position, size, UV coordinates, and color tint for 2D HUD rendering.
