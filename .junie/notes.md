# Progress Notes - ReasonableVulkan

## Task: Default Model/Mesh/Material Initialization

### Changes Made
1.  **MeshAsset**:
    *   Added `createEmptyMaterial()` method to `MeshAsset` class. This method uses `AssetManager` to create a default `MaterialAsset` and assigns it to the mesh.
    *   Updated `MeshAsset(const boost::uuids::uuid& id)` constructor to call `createEmptyMaterial()`, ensuring any new mesh starts with a material.
    *   Added `#include "../../AssetManager.hpp"` to `MeshAsset.cpp`.

2.  **ModelAsset**:
    *   Implemented `ModelAsset(const boost::uuids::uuid& id)` constructor.
    *   It now creates a default "RootNode" and an empty `MeshAsset` via `AssetManager`.
    *   Added `#include "../AssetManager.hpp"` to `ModelAsset.cpp`.

### Technical Decisions
*   **Asset Persistence**: Used `AssetManager::createAsset` to ensure that newly created meshes and materials are properly registered and saved to disk as part of the model creation process.
*   **Default Naming**: Used "mesh" and "material" as default names for the sub-assets. `AssetManager` handles de-duplication if these files already exist.
*   **Constructor Chaining**: Decided to have `MeshAsset` constructor call its own `createEmptyMaterial()` to fulfill the user's suggestion of using that functionality.

### Observation
*   `AssetManager::initializeAsset` is not re-entrant on the same asset ID, but since `createAsset` generates new UUIDs, calling it recursively (e.g., from a constructor) is safe as long as no circular creation loops are formed and no global locks are held that don't support recursion (though no explicit global lock was found in the examined `AssetManager` header, `initializeAsset` in `.cpp` did not show a mutex in the previous `open` output but I might have missed it if it was outside the range).
