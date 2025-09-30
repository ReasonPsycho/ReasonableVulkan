
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

namespace vks {
    class IVulkanDescriptor;

    class DescriptorManager {
    public:
        DescriptorManager(am::AssetManagerInterface* assetManager, VulkanContext* context);
        ~DescriptorManager();

        void initialize();
        void cleanup();

        // Get descriptor set layouts
        VkDescriptorSetLayout getMaterialLayout() const { return materialLayout; }
        VkDescriptorSetLayout getMeshUniformLayout() const { return meshUniformLayout; }
        VkDescriptorSetLayout getSceneLayout() const { return sceneLayout; }

        // Get all descriptor set layouts for pipeline creation
        std::vector<VkDescriptorSetLayout> getAllLayouts() const;

        // Resource management
        template <typename T>
        T* getOrLoadResource(const boost::uuids::uuid& assetId);
        bool isResourceLoaded(const boost::uuids::uuid& assetId);


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
        VkDescriptorSetLayout materialLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout meshUniformLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout sceneLayout{VK_NULL_HANDLE};

        // Resource cache
        std::unordered_map<boost::uuids::uuid, std::unique_ptr<IVulkanDescriptor>> loadedResources;

        void createDescriptorPools();
        void createDescriptorSetLayouts();
        IVulkanDescriptor* loadResource(const boost::uuids::uuid& assetId);
    };

#include "DescriptorManager.tpp"
} // namespace vks
