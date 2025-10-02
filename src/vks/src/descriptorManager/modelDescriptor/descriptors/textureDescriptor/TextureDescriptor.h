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
        uint32_t layerCount;
        uint32_t index;
        VkDescriptorImageInfo descriptor;
        VkSampler sampler;

        void updateDescriptor();

        void destroy();
        void cleanup() override {};
        
        TextureDescriptor(am::TextureData& textureData,VkSampler sampler,VulkanContext& vulkanContext);
    };
}



#endif //TEXTURE_H
