
# Engine Module

## Overview

The Engine module is the core orchestrator of the ReasonableVulkan engine, providing a robust Entity-Component-System (ECS) architecture with scene management capabilities. It serves as the central hub that coordinates all game systems, manages entity lifecycles, and provides a structured approach to game object organization through hierarchical scene graphs, dynamic scripting, and integrated editor tooling.

## Features

- **Entity-Component-System (ECS) Architecture**: High-performance, data-oriented design pattern with contiguous memory layout.
- **Scene Management**: Multiple scene support with active scene switching, serialization, and prefab instantiation.
- **Hierarchical Scene Graph**: Parent-child relationships between entities with transform inheritance and dirty propagation.
- **System-based Processing**: Modular system architecture for game logic, rendering, collision, HUD, and tactical grids.
- **Component Type Safety**: Template-based component management with compile-time type checking and dynamic reflection.
- **Static Reflection & Inspector**: C++26 static reflection for component serialization, deserialization, and ImGui inspector property editing (including enum-to-name combos and asset lookups).
- **Entity Recycling**: Efficient generational entity ID management with reuse capabilities.
- **Dynamic Scripting & Hot Reloading**: Runtime script attachment and dynamic DLL reloading without restarting the engine.
- **Comprehensive Editor Infrastructure**: Command history for undo/redo, gizmo visualizers, and ImGui editor integration.

## Architecture & Class Reference

### 1. Core ECS Framework

#### `Engine`
The central engine singleton (`Engine::getInstance()`):
- **Scene Management**: Creates, activates, switches, and unloads multiple `Scene` instances.
- **Main Loop Coordination**: Drives frame updates, delta time calculations, and system dispatch.
- **Subsystem Integration**: Connects platform windowing, graphics backends (`gfx::GraphicsEngine`), asset management (`am::AssetManager`), and editor tooling.

#### `Scene`
Container encapsulating all state for a single active or background world:
- **Entity Lifecycle**: Creates, clones, and destroys entities (`CreateEntity()`, `DestroyEntity()`).
- **Component Storage**: Houses typed `ComponentArray<T>` instances for every registered component type.
- **System Orchestration**: Instantiates and updates scene systems (`RenderSystem`, `TransformSystem`, `ScriptSystem`, etc.).
- **Scene Graph Root**: Maintains the root `TransformNode` hierarchy for entity parent-child relationships.
- **Prefab Instantiation**: Spawns and deserializes reusable prefab templates into live entities.

#### `Entity`
A lightweight identifier (32-bit integer) representing an individual game object within a `Scene`. Acts as a key to query components across storage arrays.

#### `TransformNode`
Manages hierarchical scene graph relationships:
- **Parent-Child Hierarchy**: Links child entity nodes to parent nodes for composite objects.
- **Matrix Calculation**: Calculates local transformation matrices from translation, rotation, and scale.
- **World Transform Propagation**: Combines parent world matrices with local transformations and propagates dirty flags down the tree.

#### `ComponentArray<T>` & `IntegralComponentArray<T>`
High-performance, contiguous component storage:
- **Cache-Friendly Storage**: Stores component data in tightly packed arrays to maximize CPU cache locality during iteration.
- **Dense-to-Sparse Mapping**: Maps sparse entity IDs to packed array indices in O(1) time.
- **`IntegralComponentArray<T>`**: Specialized storage for integral or primitive component types.

#### `SystemBase` & `System<T...>`
Base templates for all game and engine systems:
- **Component Filtering**: Declares required component signatures to query matching entities.
- **Lifecycle Events**: Receives callbacks (`OnComponentAdded`, `OnEntityRemoved`) when relevant components are attached or detached.
- **Update Execution**: Provides `Update(float deltaTime)` hooks invoked every engine frame.

#### `EventDispatcher` & Events (`Events.hpp`)
Centralized publish-subscribe event bus:
- **Event Dispatching**: Broadcasts engine, scene, and window events across decoupled systems.
- **Event Handling**: Enables systems and scripts to subscribe to selection events, entity modifications, and input signals.

### 2. Core Components

#### Transform & Identity
- **`TransformComponent`**: Stores local position, rotation (Euler angles / quaternions), and scale vectors alongside cached world transformation matrices.
- **`NameComponent`**: Stores a human-readable string identifier for display in the scene hierarchy and debugging tools.
- **`TagComponent`**: Categorizes entities via tags for targeted system queries and filtering.

#### Rendering & Camera
- **`CameraComponent`**: Defines projection type (perspective vs. orthographic), field of view (FOV), near/far clipping planes, aspect ratio, and clear color.
- **`LightComponent`**: Configures light properties for directional, point, or spot lights (color, intensity, range, attenuation, inner/outer cutoffs, and shadow casting flags).
- **`MeshComponent`**: References 3D mesh assets by UUID and submesh indices.
- **`RendererComponent`**: Assigns materials, custom shader overrides, render layer masks, and visibility toggles to entities.
- **`ThumbnailComponent`**: Configures offscreen thumbnail capture parameters and texture preview handles for asset generation.

#### User Interface & 2D HUD
- **`HUDElementComponent`**: Represents 2D screen-space UI elements with anchor points, pixel offsets, dimensions, colors, and texture asset UUIDs.
- **`HUDTextComponent`**: Defines rendered text strings, font asset references, font sizes, text alignments, and color tinting.

#### Scripting & Gameplay
- **`ScriptComponent`**: Associates an entity with a user script instance (`Script`), enabling custom gameplay logic and serializable properties.
- **`TacticalGridComponent`**: Stores grid dimensions, tile sizes, cell occupancy states, and visual highlighting configurations for tactical gameplay.
- **`PawnComponent`**: Tracks turn-based pawn state, movement points, action points, and active grid coordinates.

#### Pathfinding & Navigation
- **`GridFragment`**: Defines discrete navigation grid cells with traversal costs and passability flags.
- **`Barrier`**: Marks impassable navigation boundaries and walls.
- **`Obstacle`**: Represents static or dynamic navigation blockers modifying pathfinding costs.
- **`PathfindingAgent`**: Holds agent pathfinding state, target destinations, calculated waypoints, and movement velocity.

### 3. Core Systems

#### `RenderSystem`
Bridges the ECS scene with the rendering engine (`gfx::GraphicsEngine`):
- **Camera Gathering**: Extracts active camera transformations and configures view-projection matrices.
- **Light Extraction**: Batches directional, point, and spot lights into GPU light buffers.
- **Draw Call Dispatch**: Traverses entities with `MeshComponent`, `RendererComponent`, and `TransformComponent` to submit queued draw commands.

#### `TransformSystem`
Maintains transform hierarchy consistency:
- **Hierarchical Traversal**: Evaluates dirty flags and updates world transformation matrices recursively from root to leaf entities.
- **Local/World Conversions**: Provides coordinate transformations between local object space and world space.

#### `CollisionSystem`
Provides 3D spatial queries and raycasting:
- **Screen-to-World Rays**: Unprojects 2D screen/mouse coordinates into 3D world rays using camera projection and view matrices.
- **Ray-AABB Intersection**: Performs fast ray-vs-bounding-box intersection tests against entity mesh bounds.
- **Closest Hit Resolution**: Finds the nearest intersected entity for editor picking and object interaction.

#### `HUDSystem`
Coordinates 2D user interface layout and rendering:
- **Screen Layout**: Computes absolute screen coordinates from relative anchors and offsets.
- **Text Layout**: Generates glyph quads and UV bounds for `HUDTextComponent` elements.
- **Command Dispatch**: Batches and submits `HUDDrawItem` structs to the graphics backend.

#### `EditorSystem`
Drives the in-engine level editor interface using Dear ImGui:
- **Scene Hierarchy**: Displays tree views of all entities, supporting drag-and-drop parenting and selection.
- **Inspector Panel**: Inspects and edits entity components with live reflection and undoable property modifications.
- **Viewport & Camera Control**: Manages editor camera navigation (fly, orbit, pan) and renders viewport textures.
- **Asset Browser & Console**: Integrates file browsing, prefab creation, and engine logging output.

#### `GizmoSystem`
Renders interactive 3D transformation handles:
- **Transform Manipulation**: Provides interactive translation, rotation, and scale gizmos for selected entities.
- **Gizmo Visualizers**: Invokes registered visualizers to draw wireframe debug bounds for lights, cameras, and grid cells.

#### `ScriptSystem`
Orchestrates gameplay scripting:
- **Lifecycle Management**: Invokes `OnStart()`, `OnUpdate(deltaTime)`, and `OnDestroy()` methods on attached script instances.
- **Hot Reloading Integration**: Serializes script state, swaps DLLs upon recompilation, and restores script instances seamlessly.

#### `PathfindingSystem`
Executes spatial navigation algorithms:
- **Grid Graph Construction**: Builds navigation graphs from `GridFragment`, `Barrier`, and `Obstacle` components.
- **Path Calculation**: Computes optimal paths (A* / Dijkstra) and updates `PathfindingAgent` waypoints.

#### `TacticalGridSystem` & `TacticalSceneManagerSystem`
Drives grid-based tactical mechanics:
- **Grid Visualization**: Generates dynamic grid plane visual overlays and hover feedback.
- **Turn & Action Orchestration**: Coordinates pawn movement ranges, attack targets, and turn cycles.

### 4. Tooling & Scripting Support

#### `CommandHistory` & `EditorCommands`
Implements the Command pattern for undo and redo operations:
- **Undo / Redo Stack**: Tracks executed editor commands (`CreateEntityCommand`, `DeleteEntityCommand`, `ChangePropertyCommand`, `ReparentCommand`).
- **State Restoration**: Reverts or re-applies scene graph and component changes cleanly.

#### `ScriptHotReloadManager` & `ScriptRegistry`
Powers dynamic C++ scripting:
- **`ScriptRegistry`**: Global registry mapping script names to factory functions and exposed reflection metadata.
- **`ScriptHotReloadManager`**: Monitors compiled script shared libraries (`.dll`), unloads stale binaries, loads newly built modules, and re-binds active entity scripts.

#### `ComponentGizmoRegistry` & Visualizers
Modular debug rendering:
- **`ComponentGizmoRegistry`**: Maps component types to specialized visualizers.
- **Visualizers**:
  - `CameraGizmoVisualizer`: Renders camera frustum wireframes in the editor.
  - `LightGizmoVisualizer`: Draws light cones, spheres, and directional indicators.
  - `GridFragmentGizmoVisualizer`: Renders navigation grid cell outlines and status markers.