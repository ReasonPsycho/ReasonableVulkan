//
// Created by redkc on 10/08/2025.
//

#ifndef TEXTURE_H
#define TEXTURE_H
#include "../IVulkanDescriptor.h"
#include <vulkan/vulkan_core.h>

#include "assetDatas/TextureData.h"


namespace vks
{
    class DescriptorManager;

    struct TextureDescriptor : IVulkanDescriptor {

        uint32_t width{0};
        uint32_t height{0};
        uint32_t channels{0};
        bool hasAlpha{false};
        VkImage image;
        VkImageLayout imageLayout;
        VkDeviceMemory deviceMemory;
        VkImageView view;
        uint32_t mipLevels;
        VkDescriptorImageInfo descriptor;

        void updateDescriptor();

        void destroy();
        void cleanup() override {};
        TextureDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager, am::TextureData& textureData,VulkanContext& vulkanContext);
    };
}



#endif //TEXTURE_H
