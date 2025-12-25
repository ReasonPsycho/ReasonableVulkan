
#pragma once
#include <unordered_map>
#include "Asset.hpp"
#include "AssetManagerInterface.h"
#include "assetDatas/ShaderData.h"
#include "../vulkanContext/VulkanContext.hpp"
#include "modelDescriptor/descriptors/IVulkanDescriptor.h"
#include "modelDescriptor/ModelDescriptor.h"
#include "modelDescriptor/descriptors/meshDescriptor/MeshDescriptor.h"
#include "modelDescriptor/descriptors/ShaderDescriptor/ShaderDescriptor.h"
#include "../../vks/src/base/VulkanDevice.h"
#include <glm/glm.hpp>

#include "buffers/LightBufferData.hpp"
#include "buffers/LightSSBO.hpp"
#include "buffers/SceneUBO.hpp"

namespace vks {
    class IVulkanDescriptor;

    class DescriptorManager {
    public:
        DescriptorManager(am::AssetManagerInterface* assetManager, VulkanContext* context);
        ~DescriptorManager();

        void initialize();

        void cleanup();


        void createDefaultSampler();


        // Get descriptor set layouts
        VkDescriptorSetLayout materialLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout meshUniformLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout sceneLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout lightsLayout{VK_NULL_HANDLE};

        // Get all descriptor set layouts for pipeline creation
        std::vector<VkDescriptorSetLayout> getAllLayouts() const;

        // Resource management
        template <typename T>
        T* getOrLoadResource(const boost::uuids::uuid& assetId);
        bool isResourceLoaded(const boost::uuids::uuid& assetId);

        void createSceneUBO();
        void updateSceneUBO(const glm::mat4& projection, const glm::mat4& view, glm::vec3 cameraPos);

        void createLightsData();
        void updateLightsData(
                 const std::vector<DirectionalLightBufferData>& directionalLights,
                 const std::vector<PointLightBufferData>& pointLights,
                 const std::vector<SpotLightBufferData>& spotLights);

        // Resource management
        template <typename T>
        T* getOrLoadResource(std::string path);


        am::AssetManagerInterface* assetManager;
        VulkanContext* context;

        // Descriptor pools
        VkDescriptorPool materialPool{VK_NULL_HANDLE};
        VkDescriptorPool meshPool{VK_NULL_HANDLE};
        VkDescriptorPool scenePool{VK_NULL_HANDLE};

        // Descriptor set layouts
        VkDescriptorSetLayout getMaterialLayout() const { return materialLayout; }
        VkDescriptorSetLayout getMeshUniformLayout() const { return meshUniformLayout; }
        VkDescriptorSetLayout getSceneLayout() const { return sceneLayout; }
        VkDescriptorSetLayout getLightsLayout() const { return lightsLayout; }

        //Image sampler
        VkSampler defaultSampler = VK_NULL_HANDLE;
        VkDescriptorImageInfo defaultImageInfo = {};
        VkImage defaultImage = VK_NULL_HANDLE;
        VkImageView defaultImageView = VK_NULL_HANDLE;
        VkDeviceMemory defaultImageMemory = VK_NULL_HANDLE;

        // Resource cache
        std::unordered_map<boost::uuids::uuid, std::unique_ptr<IVulkanDescriptor>> loadedResources;
        SceneUBO sceneUBO;
        LightsInfoUBO lightInfoUBO;
        LightSSBO directionalLightSSBO;
        LightSSBO pointLightSSBO;
        LightSSBO spotLightSSBO;

        int maxDirectionalLights = 4;
        int maxPointLights = 124;
        int maxSpotLights = 124;

        void createDescriptorPools();
        void createDefaultTexture();
        void createDescriptorSetLayouts();
        IVulkanDescriptor* loadResource(const boost::uuids::uuid& assetId);



    };

#include "DescriptorManager.tpp"
} // namespace vks
