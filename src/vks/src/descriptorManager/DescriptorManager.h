
#pragma once
#include <unordered_map>
#include <type_traits>
#include "Asset.hpp"
#include "AssetManagerInterface.h"
#include "assetDatas/ShaderData.h"
#include "../vulkanContext/VulkanContext.hpp"
#include "Handle.hpp"
#include "ResourcePool.hpp"
#include "modelDescriptor/descriptors/IVulkanDescriptor.h"
#include "modelDescriptor/ModelDescriptor.h"
#include "modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"
#include "modelDescriptor/descriptors/shaderProgramDescriptor/ShaderProgramDescriptor.h"
#include "modelDescriptor/descriptors/ShaderDescriptor/ShaderDescriptor.h"
#include "modelDescriptor/descriptors/textureDescriptor/TextureDescriptor.h"
#include "modelDescriptor/descriptors/materialDescriptor/MaterialDescriptor.h"
#include "../../vks/src/base/VulkanDevice.h"
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "ShaderDefinesEnum.hpp"
#include "buffers/LightBufferData.hpp"
#include "buffers/LightSSBO.hpp"
#include "buffers/ShadowMapArray.hpp"
#include "buffers/SceneUBO.hpp"

namespace vks {
    class IVulkanDescriptor;
    class ModelDescriptor;
    class MeshDescriptor;
    class MaterialDescriptor;
    class TextureDescriptor;
    class ShaderDescriptor;
    class ShaderProgramDescriptor;

    class DescriptorManager {
    public:
        DescriptorManager(am::AssetManagerInterface* assetManager, VulkanContext* context);
        ~DescriptorManager();

        void initialize();

        void cleanup();


        void createDefaultSampler();


        // Get descriptor set layouts
        VkDescriptorSetLayout pbrMaterialLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout skyboxMaterialLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout meshUniformLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout sceneLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout lightsLayout{VK_NULL_HANDLE};

        // Get all descriptor set layouts for pipeline creation
        std::vector<VkDescriptorSetLayout> getAllLayouts() const;

        // Resource pools
        ResourcePool<ModelDescriptor, gfx::ModelTag> modelPool;
        ResourcePool<ShaderProgramDescriptor, gfx::ShaderProgramTag> shaderProgramPool;
        ResourcePool<ShaderDescriptor, gfx::ShaderTag> shaderPool;
        ResourcePool<TextureDescriptor, gfx::TextureTag> texturePool;
        ResourcePool<MaterialDescriptor, gfx::MaterialTag> materialPool;
        ResourcePool<MeshDescriptor, gfx::MeshTag> meshResourcePool;

        // UUID to Handle maps
        std::unordered_map<boost::uuids::uuid, gfx::ModelHandle> uuidToModelMap;
        std::unordered_map<boost::uuids::uuid, gfx::ShaderProgramHandle> uuidToShaderProgramMap;
        std::unordered_map<boost::uuids::uuid, gfx::ShaderHandle> uuidToShaderMap;
        std::unordered_map<boost::uuids::uuid, gfx::TextureHandle> uuidToTextureMap;
        std::unordered_map<boost::uuids::uuid, gfx::MaterialHandle> uuidToMaterialMap;
        std::unordered_map<boost::uuids::uuid, gfx::MeshHandle> uuidToMeshMap;

        // Handle-based getters (O(1) direct slot index lookup)
        [[nodiscard]] ModelDescriptor* getModel(gfx::ModelHandle handle) const;
        [[nodiscard]] ShaderProgramDescriptor* getShaderProgram(gfx::ShaderProgramHandle handle) const;
        [[nodiscard]] ShaderDescriptor* getShader(gfx::ShaderHandle handle) const;
        [[nodiscard]] TextureDescriptor* getTexture(gfx::TextureHandle handle) const;
        [[nodiscard]] MaterialDescriptor* getMaterial(gfx::MaterialHandle handle) const;
        [[nodiscard]] MeshDescriptor* getMesh(gfx::MeshHandle handle) const;

        // Handle-returning loaders
        gfx::ModelHandle getOrLoadModel(const boost::uuids::uuid& assetId);
        gfx::ShaderProgramHandle getOrLoadShaderProgram(const boost::uuids::uuid& assetId);
        gfx::ShaderHandle getOrLoadShader(const boost::uuids::uuid& assetId);
        gfx::TextureHandle getOrLoadTexture(const boost::uuids::uuid& assetId);
        gfx::MaterialHandle getOrLoadMaterial(const boost::uuids::uuid& assetId);
        gfx::MeshHandle getOrLoadMesh(const boost::uuids::uuid& assetId);

        // Name-based handle-returning loaders
        gfx::ModelHandle getOrLoadModel(const std::string& lookUpName);
        gfx::ShaderProgramHandle getOrLoadShaderProgram(const std::string& lookUpName);
        gfx::ShaderHandle getOrLoadShader(const std::string& lookUpName);
        gfx::TextureHandle getOrLoadTexture(const std::string& lookUpName);
        gfx::MaterialHandle getOrLoadMaterial(const std::string& lookUpName);
        gfx::MeshHandle getOrLoadMesh(const std::string& lookUpName);

        // UUID to Handle lookup
        [[nodiscard]] gfx::ModelHandle getModelHandle(const boost::uuids::uuid& assetId) const;
        [[nodiscard]] gfx::ShaderProgramHandle getShaderProgramHandle(const boost::uuids::uuid& assetId) const;
        [[nodiscard]] gfx::ShaderHandle getShaderHandle(const boost::uuids::uuid& assetId) const;
        [[nodiscard]] gfx::TextureHandle getTextureHandle(const boost::uuids::uuid& assetId) const;
        [[nodiscard]] gfx::MaterialHandle getMaterialHandle(const boost::uuids::uuid& assetId) const;
        [[nodiscard]] gfx::MeshHandle getMeshHandle(const boost::uuids::uuid& assetId) const;

        // Compatibility resource management
        template <typename T>
        T* getOrLoadResource(const boost::uuids::uuid& assetId);
        template <typename T>
        T* getOrLoadResource(std::string lookUpName);
        bool isResourceLoaded(const boost::uuids::uuid& assetId);

        void createSceneUBO();
        void updateSceneUBO(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, glm::vec3 cameraPos);

        void createLightsData();
        void updateLightsData(
                 const std::vector<DirectionalLightBufferData>& directionalLights,
                 const std::vector<PointLightBufferData>& pointLights,
                 const std::vector<SpotLightBufferData>& spotLights,
                 float farPlane);

        am::AssetManagerInterface* assetManager;
        VulkanContext* context;

        // Descriptor pools
        VkDescriptorPool pbrMaterialPool{VK_NULL_HANDLE};
        VkDescriptorPool skyboxMaterialPool{VK_NULL_HANDLE};
        VkDescriptorPool meshPool{VK_NULL_HANDLE};
        VkDescriptorPool scenePool{VK_NULL_HANDLE};
        VkDescriptorPool lightPool{VK_NULL_HANDLE};

        // Descriptor set layouts
        VkDescriptorSetLayout getPbrMaterialLayout() const { return pbrMaterialLayout; }
        VkDescriptorSetLayout getSkyboxMaterialLayout() const { return skyboxMaterialLayout; }
        VkDescriptorSetLayout getMeshUniformLayout() const { return meshUniformLayout; }
        VkDescriptorSetLayout getSceneLayout() const { return sceneLayout; }
        VkDescriptorSetLayout getLightsLayout() const { return lightsLayout; }
        std::vector<VkDescriptorSetLayout> getLayoutsFromEnums(std::vector<ShaderDefinesEnum> definitions);

        //Image sampler
        VkSampler defaultSampler = VK_NULL_HANDLE;
        VkDescriptorImageInfo defaultImageInfo = {};
        VkImage defaultImage = VK_NULL_HANDLE;
        VkImageView defaultImageView = VK_NULL_HANDLE;
        VkDeviceMemory defaultImageMemory = VK_NULL_HANDLE;

        //Cube sampler
        VkSampler cubeSampler = VK_NULL_HANDLE;
        VkDescriptorImageInfo cubeImageInfo = {};
        VkImage cubeImage = VK_NULL_HANDLE;
        VkImageView cubeImageView = VK_NULL_HANDLE;
        VkDeviceMemory cubeImageMemory = VK_NULL_HANDLE;

        std::vector<SceneUBO> sceneUBOs;
        LightsInfoUBO lightInfoUBO;
        LightSSBO directionalLightSSBO;
        LightSSBO pointLightSSBO;
        LightSSBO spotLightSSBO;
        ShadowMapArray shadowMapArray;
        ShadowMapArray cubeMapShadowMapArray;

        int maxDirectionalLights = 4;
        int maxPointLights = 124;
        int maxSpotLights = 124;

        void updateShadowDescriptorSet(VkImageView directionalView, VkImageView pointView, VkImageView spotView);

        void createDescriptorPools();
        void createDefaultTexture();
        void createDefaultCubeTexture();
        void createDescriptorSetLayouts();
    };

#include "DescriptorManager.tpp"
} // namespace vks
