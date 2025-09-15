#ifndef MATERIALDATA_H
#define MATERIALDATA_H

#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "AssetInfo.hpp"

namespace am {

    struct MaterialData {
        // Base color texture or diffuse color
        std::shared_ptr<am::AssetInfo> baseColorTexture;
        glm::vec4 baseColorFactor = glm::vec4(1.0f); // Albedo factor

        // Legacy diffuse texture
        std::shared_ptr<am::AssetInfo> diffuseTexture;

        // Metallic-Roughness workflow
        std::shared_ptr<am::AssetInfo> metallicRoughnessTexture;
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;

        // Specular-Glossiness workflow (optional)
        std::shared_ptr<am::AssetInfo> specularGlossinessTexture;
        glm::vec3 specularFactor = glm::vec3(1.0f); // Specular color
        glm::vec3 diffuseFactor = glm::vec3(1.0f); // Diffuse factor for specular-glossiness workflow
        float glossinessFactor = 1.0f;

        // Normal map
        std::shared_ptr<am::AssetInfo> normalTexture;

        // Occlusion (Ambient Occlusion)
        std::shared_ptr<am::AssetInfo> occlusionTexture;
        float occlusionStrength = 1.0f;

        // Emissive
        std::shared_ptr<am::AssetInfo> emissiveTexture;
        glm::vec3 emissiveFactor = glm::vec3(1.0f);

        // Alpha properties
        float alphaCutoff = 1.0f;
        bool isOpaque = true;

        // Workflow type (Metallic-Roughness or Specular-Glossiness)
        bool useSpecularGlossiness = false;
    };

} // namespace am

#endif // MATERIALDATA_H