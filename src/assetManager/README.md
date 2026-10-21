
# Asset Manager Module

## Overview

The Asset Manager module is a core component of the ReasonableVulkan engine that provides centralized asset loading, management, and caching functionality. It implements a singleton pattern for global access and uses a factory-based approach for extensible asset loading, automatic metadata synchronization, dependency tracking, and lazy resource hydration.

## Features

- **Centralized Asset Management**: Single point of access for all game and engine assets via `am::AssetManager::getInstance()`.
- **UUID-based Asset Identification**: Assets are identified by persistent Boost UUIDs preserved across sessions.
- **Factory Pattern**: Extensible asset loading architecture allowing registration of custom asset factories.
- **Lazy Loading & Memory Caching**: Assets load on-demand when first dereferenced and cache parsed GPU/CPU data in memory.
- **Path & Name Indexing**: Fast bidirectional lookups between asset file paths, lookup names, and UUIDs.
- **Content Hash Validation**: Change detection through SHA/content hashing to avoid redundant re-parsing.
- **Integrated File Browser**: Built-in ImGui asset and file system explorer with search and preview capabilities.
- **GLSL Include & Shader Processing**: Automatic shader preprocessing, GLSL include resolution, reflection scanning, and SPIR-V compilation.

## Architecture & Class Reference

### 1. Core Manager & Interfaces

#### `AssetManager`
The primary singleton orchestrating all asset operations (`am::AssetManager::getInstance()`):
- **Asset Registration**: Scans project directories and registers assets from files and `.meta` sidecars.
- **Factory Registry**: Registers, configures, and invokes typed `AssetFactory` functions for models, textures, shaders, materials, fonts, scenes, and prefabs.
- **UUID & Path Lookup**: Resolves assets by `boost::uuids::uuid`, file path string, or registered lookup name.
- **Caching & Lifecycle**: Manages loaded asset memory, eviction, change detection, and reload triggers.

#### `AssetManagerInterface`
Abstract interface decoupling the rendering engine (`vks`) and other subsystems from the concrete asset manager implementation:
- Exposes generic asset queries, type resolution, and raw asset data access.

#### `Asset` (Abstract Base Class)
Base class for all concrete asset instances:
- **Identification**: Stores unique `boost::uuids::uuid`, asset type enum (`AssetType`), and source file path.
- **Content Hashing**: Calculates and tracks content hashes to detect disk modifications.
- **Data Access**: Virtual methods to retrieve typed asset data payload structs (`getRawData()`).

#### `AssetInfo`
Metadata descriptor wrapping registered assets:
- **Metadata Management**: Holds UUID, asset name, file path, asset type, and load status flags.
- **Lazy Loading**: Manages lazy loading proxy logic via `getAsset()`, instantiating the underlying `Asset` on first access.

#### `AssetFactoryData`
Context structure passed to asset loader factories during instantiation:
- Encapsulates target file paths, asset type specifications, parent asset references, and auxiliary parameters (e.g., Assimp submesh indices).

#### `UUIDManager`
Utility class for UUID generation, parsing, serialization, and string conversions using Boost.UUID.

### 2. Concrete Asset Implementations

#### `ModelAsset`
Parses 3D model files (`.fbx`, `.obj`, `.gltf`) using Assimp:
- Extracts scene node hierarchies (`Node`), mesh indices, bone structures, and embedded material references.
- Coordinates submesh asset creation during asset import.

#### `MeshAsset`
Represents individual geometry mesh buffers:
- Encapsulates vertex data (`VertexAsset`: positions, normals, tangents, UV coordinates) and index buffers.
- Computes axis-aligned bounding boxes (AABB) for spatial queries and frustum culling.

#### `MaterialAsset`
Loads and serializes PBR material parameters:
- Manages albedo tint, metallic factor, roughness factor, emissive color, and normal map intensity.
- Links dependent texture asset UUIDs for albedo, normal, metallic-roughness, emissive, and ambient occlusion maps.

#### `TextureAsset`
Loads 2D image textures (`.png`, `.jpg`, `.tga`):
- Uses `stb_image` to parse image buffers, dimensions, channels (RGBA), and bit depth.
- Supports cubemaps, normal maps, and HDR environment textures.
- Fails on import if the source image file cannot be read from disk.

#### `ShaderAsset`
Compiles and manages individual GLSL shader stages (`.vert`, `.frag`, `.spv`, `.spdv`):
- Uses `glslang` for runtime compilation of GLSL source to SPIR-V bytecode.
- Integrates with `ShaderIncluder` to support `#include` directives.
- Analyzes define markers (e.g., `SCENE_UBO_GLSL`, `MODEL_PC_GLSL`) for pipeline reflection.

#### `ShaderProgramAsset`
Links paired shader stages into complete shader programs:
- Loads `.shader` JSON definitions linking vertex and fragment shader stages.
- Computes aggregate descriptor layout requirements and pipeline flags for the graphics backend.

#### `FontAsset`
Parses TrueType (`.ttf`) and bitmap fonts:
- Generates font texture atlases and computes glyph metric tables for text rendering in `HUDSystem`.

#### `SceneAsset`
Encapsulates serialized ECS scene hierarchies:
- Reads and writes scene entities, transform nodes, component configurations, and environment settings.

#### `PrefabAsset`
Represents reusable entity templates:
- Stores serialized entity definitions, components, and child hierarchies for runtime instantiation.

#### `ConfigAsset`
Handles engine and project configuration files:
- Serializes display resolutions, graphics settings, editor preferences, and keybindings.

### 3. Asset Data Structures (`include/assetDatas/`)

- **`ModelData` & `Node`**: In-memory representation of 3D scene graphs with child nodes and mesh bindings.
- **`MeshData` & `VertexAsset`**: Contiguous vertex arrays (`VertexAsset` struct with positions, normals, tangents, UVs) and 32-bit indices.
- **`MaterialData`**: PBR material property container mapping texture UUIDs and scalar factors.
- **`TextureData`**: Raw pixel byte vectors, dimensions (width, height), format, and channel count.
- **`ShaderData`**: SPIR-V binary bytecode, shader stage type, entry point name, and reflected `#define` tokens.
- **`ShaderProgramData`**: Combined vertex/fragment shader handles, reflection metadata, and descriptor set requirements.
- **`FontData`**: Glyph metrics, advance tables, atlas image data, and baseline offsets.

### 4. Utilities & Helpers

#### `ImguiFileBrowser` & `ImguiFileBrowserInterface`
Integrated asset browser panel:
- Provides visual directory tree navigation, file searching, asset dragging, and thumbnail previews.

#### `ShaderIncluder`
Custom `glslang` include handler:
- Resolves relative and system `#include` statements in GLSL shader source files.

#### `AssimpGLMHelpers` & `JsonHelpers`
Mathematics and serialization adapters:
- **`AssimpGLMHelpers`**: Converts Assimp matrices and vectors (`aiMatrix4x4`, `aiVector3D`, `aiQuaternion`) to GLM types.
- **`JsonHelpers`**: Helper utilities for nlohmann/json serialization and deserialization of engine types.

#### `AssetReflection`
Runtime type reflection utilities for asset inspection and editor serialization.