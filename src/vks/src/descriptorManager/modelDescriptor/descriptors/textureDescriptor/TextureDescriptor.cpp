//
// Created by redkc on 10/08/2025.
//

#include "TextureDescriptor.h"
#include <spdlog/spdlog.h>
#include "../../../DescriptorManager.h"


void vks::TextureDescriptor::updateDescriptor() {
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}

void vks::TextureDescriptor::destroy() {
	if (device) {
		vkDestroyImageView(device, view, nullptr);
		vkDestroyImage(device, image, nullptr);
		vkFreeMemory(device, deviceMemory, nullptr);
	}
}

vks::TextureDescriptor::TextureDescriptor(const boost::uuids::uuid& assetId, DescriptorManager* assetHandleManager, am::TextureData& textureData,VulkanContext& vulkanContext)
    : IVulkanDescriptor(assetId, vulkanContext) {
    this->width = textureData.width;
    this->height = textureData.height;
    this->channels = textureData.channels;
    this->hasAlpha = textureData.hasAlpha;

    // Set format based on channels
    VkFormat format = (channels == 4) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8_UNORM;

    uint32_t imageWidth = width;
    uint32_t imageHeight = height;

    if (textureData.type == am::TextureType::TextureCube) {
        if (width != height) {
            // Assume 4x3 grid if not square
            imageWidth = width / 4;
            imageHeight = height / 3;
            if (imageWidth != imageHeight) {
                spdlog::warn("Cube map face dimensions are not square: {}x{}", imageWidth, imageHeight);
            }
        }
    }

    mipLevels = static_cast<uint32_t>(floor(log2(std::max(imageWidth, imageHeight))) + 1.0);

    // Verify format support
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(vulkanContext.getPhysicalDevice(), format, &formatProperties);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
    assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // Create staging buffer using VulkanContext utility
    vulkanContext.createBuffer(
        textureData.pixels.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory);

    // Copy data to staging buffer
    void* data;
    vkMapMemory(vulkanContext.getDevice(), stagingMemory, 0, textureData.pixels.size(), 0, &data);
    memcpy(data, textureData.pixels.data(), textureData.pixels.size());
    vkUnmapMemory(vulkanContext.getDevice(), stagingMemory);

    // Create the image
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent = {imageWidth, imageHeight, 1};
    imageCreateInfo.mipLevels = mipLevels;
    if (textureData.type == am::TextureType::TextureCube) {
        imageCreateInfo.arrayLayers = 6;
        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    } else {
        imageCreateInfo.arrayLayers = 1;
    }
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK_RESULT(vkCreateImage(vulkanContext.getDevice(), &imageCreateInfo, nullptr, &image));

    // Allocate memory for the image
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(vulkanContext.getDevice(), image, &memReqs);

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReqs.size;
    memAllocInfo.memoryTypeIndex = vulkanContext.findMemoryType(
        memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK_RESULT(vkAllocateMemory(vulkanContext.getDevice(), &memAllocInfo, nullptr, &deviceMemory));
    VK_CHECK_RESULT(vkBindImageMemory(vulkanContext.getDevice(), image, deviceMemory, 0));

    // Transition image layout and copy data
    VkCommandBuffer cmdBuffer = vulkanContext.beginSingleTimeCommands(QueueType::Graphics);

    // Transition to transfer destination layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    if (textureData.type == am::TextureType::TextureCube) {
        barrier.subresourceRange.layerCount = 6;
    } else {
        barrier.subresourceRange.layerCount = 1;
    }
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // Copy buffer to image
    if (textureData.type == am::TextureType::TextureCube && width != height) {
        std::vector<VkBufferImageCopy> copyRegions;
        // Standard 4x3 layout:
        //     [+Y]
        // [-X][+Z][+X][-Z]
        //     [-Y]
        // Map to Vulkan faces: 0:+X, 1:-X, 2:+Y, 3:-Y, 4:+Z, 5:-Z

        struct FacePos { int row; int col; };
        FacePos facePositions[6] = {
            {1, 2}, // +X
            {1, 0}, // -X
            {0, 1}, // +Y
            {2, 1}, // -Y
            {1, 1}, // +Z
            {1, 3}  // -Z
        };

        for (uint32_t i = 0; i < 6; i++) {
            VkBufferImageCopy region{};
            region.bufferOffset = (facePositions[i].row * imageHeight * width + facePositions[i].col * imageWidth) * 4;
            region.bufferRowLength = width; // Crucial: row length is the full width of the source grid
            region.bufferImageHeight = height;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = i;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {imageWidth, imageHeight, 1};
            copyRegions.push_back(region);
        }

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
    } else {
        VkBufferImageCopy copyRegion{};
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        if (textureData.type == am::TextureType::TextureCube) {
            copyRegion.imageSubresource.layerCount = 6;
        } else {
            copyRegion.imageSubresource.layerCount = 1;
        }
        copyRegion.imageExtent = {imageWidth, imageHeight, 1};

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
    }

    // Transition to shader read layout
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    vulkanContext.endSingleTimeCommands(cmdBuffer, QueueType::Graphics);

    // Clean up staging resources
    vkDestroyBuffer(vulkanContext.getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanContext.getDevice(), stagingMemory, nullptr);

    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    if (textureData.type == am::TextureType::TextureCube) {
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    } else {
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    if (textureData.type == am::TextureType::TextureCube) {
        viewInfo.subresourceRange.layerCount = 6;
    } else {
        viewInfo.subresourceRange.layerCount = 1;
    }

    VK_CHECK_RESULT(vkCreateImageView(vulkanContext.getDevice(), &viewInfo, nullptr, &view));

    if (textureData.type == am::TextureType::Texture2D)
    {
        descriptor.sampler = assetHandleManager->defaultSampler;
    }else
    {
        descriptor.sampler = assetHandleManager->cubeSampler;
    }
    // Update descriptor
    descriptor.imageView = view;
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
