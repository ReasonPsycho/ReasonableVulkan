
# Asset Manager Module

## Overview

The Asset Manager module is a core component of the ReasonableVulkan engine that provides centralized asset loading, management, and caching functionality. It implements a singleton pattern for global access and uses a factory-based approach for extensible asset loading.

## Features

- **Centralized Asset Management**: Single point of access for all game assets
- **UUID-based Asset Identification**: Each asset is uniquely identified using Boost UUID
- **Factory Pattern**: Extensible asset loading system supporting multiple asset types
- **Lazy Loading**: Assets are loaded on-demand when first requested
- **Path-based Registration**: Assets can be registered and retrieved by file path
- **Content Hash Validation**: Assets track content changes through hash comparison
- **Memory Management**: Automatic cleanup and memory management of loaded assets

## Supported Asset Types

The module currently supports the following asset types:

- **Model** (`.fbx` files) - 3D models and scenes
- **Texture** (`.png` files) - Image textures
- **Shader** (`.spdv` files) - Shader programs
- **Mesh** - Individual mesh data
- **Material** - Material definitions
- **Animation** - Animation sequences
- **Animator** - Animation controllers

## Architecture

### Core Classes

#### `AssetManager`
The main singleton class that orchestrates all asset operations:
- Registers and manages asset factories
- Handles asset registration from file paths
- Provides UUID-based asset lookup
- Manages asset metadata and loaded instances

#### `Asset` (Abstract Base Class)
Base class for all asset types with common functionality:
- Content hash calculation for change detection
- Type identification
- Generic data access methods

#### `AssetInfo`
Metadata container for registered assets:
- Stores asset identification (UUID, path, type)
- Tracks loading state and content hash
- Provides lazy loading through `getAsset()` method

#### `AssetFactoryData`
Context structure passed to asset factories during creation:
- File path information
- Asset type specification
- Additional parameters (e.g., Assimp mesh index)