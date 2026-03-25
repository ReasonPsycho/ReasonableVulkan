#include "DescriptorManager.h"
#include <stdexcept>

#include "buffers/LightBufferData.hpp"


namespace vks
{
    DescriptorManager::DescriptorManager(am::AssetManagerInterface* assetManager, VulkanContext* context)
        : assetManager(assetManager)
          , context(context)
    {
    }

    DescriptorManager::~DescriptorManager()
    {
        cleanup();
    }

    void DescriptorManager::initialize()
    {
        createDescriptorPools();
        createDescriptorSetLayouts();
        createDefaultSampler();
        createSceneUBO();
        createLightsData();
    }

 void DescriptorManager::cleanup()
    {
        auto device = context->getDevice();

        for (auto& sceneUBO : sceneUBOs) {
            if (sceneUBO.buffer.buffer != VK_NULL_HANDLE)
            {
                if (sceneUBO.buffer.mapped)
                {
                    vkUnmapMemory(device, sceneUBO.buffer.memory);
                }
                vkDestroyBuffer(device, sceneUBO.buffer.buffer, nullptr);
                vkFreeMemory(device, sceneUBO.buffer.memory, nullptr);
            }
        }
        sceneUBOs.clear();

        if (lightInfoUBO.buffer.buffer != VK_NULL_HANDLE)
        {
            if (lightInfoUBO.buffer.mapped)
            {
                vkUnmapMemory(device, lightInfoUBO.buffer.memory);
            }
            vkDestroyBuffer(device, lightInfoUBO.buffer.buffer, nullptr);
            vkFreeMemory(device, lightInfoUBO.buffer.memory, nullptr);
        }

        if (directionalLightSSBO.buffer.buffer != VK_NULL_HANDLE) {
            if (directionalLightSSBO.buffer.mapped) {
                vkUnmapMemory(device, directionalLightSSBO.buffer.memory);
            }
            vkDestroyBuffer(device, directionalLightSSBO.buffer.buffer, nullptr);
            vkFreeMemory(device, directionalLightSSBO.buffer.memory, nullptr);
        }

        if (pointLightSSBO.buffer.buffer != VK_NULL_HANDLE) {
            if (pointLightSSBO.buffer.mapped) {
                vkUnmapMemory(device, pointLightSSBO.buffer.memory);
            }
            vkDestroyBuffer(device, pointLightSSBO.buffer.buffer, nullptr);
            vkFreeMemory(device, pointLightSSBO.buffer.memory, nullptr);
        }

        if (spotLightSSBO.buffer.buffer != VK_NULL_HANDLE) {
            if (spotLightSSBO.buffer.mapped) {
                vkUnmapMemory(device, spotLightSSBO.buffer.memory);
            }
            vkDestroyBuffer(device, spotLightSSBO.buffer.buffer, nullptr);
            vkFreeMemory(device, spotLightSSBO.buffer.memory, nullptr);
        }

        if (shadowMapArray.buffer.buffer != VK_NULL_HANDLE) {
            if (shadowMapArray.buffer.mapped) {
                vkUnmapMemory(device, shadowMapArray.buffer.memory);
            }
            vkDestroyBuffer(device, shadowMapArray.buffer.buffer, nullptr);
            vkFreeMemory(device, shadowMapArray.buffer.memory, nullptr);
        }

        if (cubeMapShadowMapArray.buffer.buffer != VK_NULL_HANDLE) {
            if (cubeMapShadowMapArray.buffer.mapped) {
                vkUnmapMemory(device, cubeMapShadowMapArray.buffer.memory);
            }
            vkDestroyBuffer(device, cubeMapShadowMapArray.buffer.buffer, nullptr);
            vkFreeMemory(device, cubeMapShadowMapArray.buffer.memory, nullptr);
        }

        if (defaultSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, defaultSampler, nullptr);
            defaultSampler = VK_NULL_HANDLE;
        }

        // Clear resource cache
        loadedResources.clear();

        // Destroy descriptor sets layouts
        if (pbrMaterialLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, pbrMaterialLayout, nullptr);
        }
        if (meshUniformLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, meshUniformLayout, nullptr);
        }
        if (sceneLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, sceneLayout, nullptr);
        }
        if (lightsLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, lightsLayout, nullptr);
        }

        // Destroy descriptor pools
        if (pbrMaterialPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, pbrMaterialPool, nullptr);
        }
        if (meshPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, meshPool, nullptr);
        }
        if (scenePool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, scenePool, nullptr);
        }
    }

    void DescriptorManager::createDescriptorPools()
    {
        std::vector<VkDescriptorPoolSize> materialPoolSizes = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 100},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
        };

        VkDescriptorPoolCreateInfo materialPoolInfo{};
        materialPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        materialPoolInfo.poolSizeCount = static_cast<uint32_t>(materialPoolSizes.size());
        materialPoolInfo.pPoolSizes = materialPoolSizes.data();
        materialPoolInfo.maxSets = 100;

        if (vkCreateDescriptorPool(context->getDevice(), &materialPoolInfo, nullptr, &pbrMaterialPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create material descriptor pool!");
        }

        // Mesh pool (no changes needed)
        VkDescriptorPoolSize meshPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000};
        VkDescriptorPoolCreateInfo meshPoolInfo{};
        meshPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        meshPoolInfo.poolSizeCount = 1;
        meshPoolInfo.pPoolSizes = &meshPoolSize;
        meshPoolInfo.maxSets = 1000;

        if (vkCreateDescriptorPool(context->getDevice(), &meshPoolInfo, nullptr, &meshPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create mesh descriptor pool!");
        }

        // Scene pool
        std::vector<VkDescriptorPoolSize> scenePoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10}, // For camera and lighting uniforms
        };

        VkDescriptorPoolCreateInfo scenePoolInfo{};
        scenePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        scenePoolInfo.poolSizeCount = static_cast<uint32_t>(scenePoolSizes.size());
        scenePoolInfo.pPoolSizes = scenePoolSizes.data();
        scenePoolInfo.maxSets = 10;

        if (vkCreateDescriptorPool(context->getDevice(), &scenePoolInfo, nullptr, &scenePool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create scene descriptor pool!");
        }
    }


    void DescriptorManager::createDefaultTexture()
    {
        auto device = context->getDevice();

        // Create a 1x1 white texture (RGBA: 255, 255, 255, 255)
        uint32_t whitePixel = 0xFFFFFFFF;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        context->createBuffer(
            sizeof(whitePixel),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);

        // Copy white pixel data to staging buffer
        void* data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, sizeof(whitePixel), 0, &data));
        memcpy(data, &whitePixel, sizeof(whitePixel));
        vkUnmapMemory(device, stagingMemory);

        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {1, 1, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &defaultImage));

        // Allocate memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, defaultImage, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context->findMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &defaultImageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, defaultImage, defaultImageMemory, 0));

        // Transition image layout and copy data using GRAPHICS queue
        VkCommandBuffer cmdBuffer = context->beginSingleTimeCommands(QueueType::Graphics);

        // Transition to transfer destination optimal
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = defaultImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {1, 1, 1};

        vkCmdCopyBufferToImage(
            cmdBuffer,
            stagingBuffer,
            defaultImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        // Transition to shader read optimal
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        context->endSingleTimeCommands(cmdBuffer, QueueType::Graphics);

        // Cleanup staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = defaultImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &defaultImageView));
    }
    void DescriptorManager::createDefaultSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.maxLod = static_cast<float>(0);
        samplerInfo.maxAnisotropy = 8.0f;
        samplerInfo.anisotropyEnable = VK_TRUE;

        VK_CHECK_RESULT(vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &defaultSampler));

        // Create a cube sampler as well
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        VK_CHECK_RESULT(vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &cubeSampler));

        createDefaultTexture();
        createDefaultCubeTexture();

        // Setup default image info
        defaultImageInfo.sampler = defaultSampler;
        defaultImageInfo.imageView = defaultImageView;
        defaultImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        cubeImageInfo.sampler = cubeSampler;
        cubeImageInfo.imageView = cubeImageView;
        cubeImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }


    void DescriptorManager::createDefaultCubeTexture()
    {
        auto device = context->getDevice();

        // Create a 1x1x6 white cube map texture
        uint32_t whitePixels[6] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        context->createBuffer(
            sizeof(whitePixels),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);

        // Copy white pixels data to staging buffer
        void* data;
        VK_CHECK_RESULT(vkMapMemory(device, stagingMemory, 0, sizeof(whitePixels), 0, &data));
        memcpy(data, whitePixels, sizeof(whitePixels));
        vkUnmapMemory(device, stagingMemory);

        // Create image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { 1, 1, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VK_CHECK_RESULT(vkCreateImage(device, &imageInfo, nullptr, &cubeImage));

        // Allocate memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, cubeImage, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = context->findMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &cubeImageMemory));
        VK_CHECK_RESULT(vkBindImageMemory(device, cubeImage, cubeImageMemory, 0));

        // Transition image layout and copy data using GRAPHICS queue
        VkCommandBuffer cmdBuffer = context->beginSingleTimeCommands(QueueType::Graphics);

        // Transition to transfer destination optimal
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = cubeImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 6;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { 1, 1, 1 };

        vkCmdCopyBufferToImage(
            cmdBuffer,
            stagingBuffer,
            cubeImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

        // Transition to shader read optimal
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        context->endSingleTimeCommands(cmdBuffer, QueueType::Graphics);

        // Cleanup staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = cubeImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;

        VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &cubeImageView));
    }


        void DescriptorManager::createDescriptorSetLayouts()
        {
            // Set 0: Scene descriptors (Camera UBO + Light Info UBO)
            std::vector<VkDescriptorSetLayoutBinding> sceneBindings = {
                    {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            };

            VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
            sceneLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            sceneLayoutInfo.bindingCount = static_cast<uint32_t>(sceneBindings.size());
            sceneLayoutInfo.pBindings = sceneBindings.data();

            if (vkCreateDescriptorSetLayout(context->getDevice(), &sceneLayoutInfo, nullptr, &sceneLayout) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create scene descriptor set layout!");
            }

            // Set 1: pbr Material descriptors (sampler + images)
            std::vector<VkDescriptorSetLayoutBinding> pbrMaterialBindings = {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
            };

            VkDescriptorSetLayoutCreateInfo pbrMaterialLayoutInfo{};
            pbrMaterialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            pbrMaterialLayoutInfo.bindingCount = static_cast<uint32_t>(pbrMaterialBindings.size());
            pbrMaterialLayoutInfo.pBindings = pbrMaterialBindings.data();

            if (vkCreateDescriptorSetLayout(context->getDevice(), &pbrMaterialLayoutInfo, nullptr, &pbrMaterialLayout) !=
                VK_SUCCESS)
            {
                throw std::runtime_error("failed to create material descriptor set layout!");
            }

        // Set pbr: pbr Material descriptors (sampler + images)
        std::vector<VkDescriptorSetLayoutBinding> skyboxMaterialBindings = {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },{
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
        };

        VkDescriptorSetLayoutCreateInfo skyboxMaterialLayoutInfo{};
        skyboxMaterialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        skyboxMaterialLayoutInfo.bindingCount = static_cast<uint32_t>(skyboxMaterialBindings.size());
        skyboxMaterialLayoutInfo.pBindings = skyboxMaterialBindings.data();

        if (vkCreateDescriptorSetLayout(context->getDevice(), &skyboxMaterialLayoutInfo, nullptr, &skyboxMaterialLayout) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create material descriptor set layout!");
        }

            // Set 2: Mesh/Vertex shader uniforms
            VkDescriptorSetLayoutBinding meshBinding{};
            meshBinding.binding = 0;
            meshBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            meshBinding.descriptorCount = 1;
            meshBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkDescriptorSetLayoutCreateInfo meshLayoutInfo{};
            meshLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            meshLayoutInfo.bindingCount = 1;
            meshLayoutInfo.pBindings = &meshBinding;

            if (vkCreateDescriptorSetLayout(context->getDevice(), &meshLayoutInfo, nullptr, &meshUniformLayout) !=
                VK_SUCCESS)
            {
                throw std::runtime_error("failed to create mesh descriptor set layout!");
            }

            // Set 3: Lights descriptors (All light types as storage buffers + Shadow Maps)
            std::vector<VkDescriptorSetLayoutBinding> lightsBindings = {
                {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 2,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 3,
                    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 4,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1, // Single sampler2DArray for directional shadows
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 5,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1, // Single samplerCubeArray for point shadows
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 6,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    .descriptorCount = 1, // Single sampler2DArray for spot shadows
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                },
                {
                    .binding = 7,
                    .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
                    .descriptorCount = 1, // Sampler for shadows
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .pImmutableSamplers = nullptr
                }
            };

            VkDescriptorSetLayoutCreateInfo lightsLayoutInfo{};
            lightsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            lightsLayoutInfo.bindingCount = static_cast<uint32_t>(lightsBindings.size());
            lightsLayoutInfo.pBindings = lightsBindings.data();

            if (vkCreateDescriptorSetLayout(context->getDevice(), &lightsLayoutInfo, nullptr, &lightsLayout) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create lights descriptor set layout!");
            }
        }


    std::vector<VkDescriptorSetLayout> DescriptorManager::getAllLayouts() const
    {
        return {sceneLayout, pbrMaterialLayout, meshUniformLayout, lightsLayout};  // Order: 0, 1, 2, 3
    }

    bool DescriptorManager::isResourceLoaded(const boost::uuids::uuid& assetId)
    {
        return loadedResources.find(assetId) != loadedResources.end();
    }


    void DescriptorManager::createSceneUBO()
    {
        uint32_t maxCameras = 4; // Scale up to 4 cameras for now
        sceneUBOs.resize(maxCameras);

        for (uint32_t i = 0; i < maxCameras; i++) {
            VkDeviceSize bufferSize = sizeof(SceneUBO::UniformBlock);

            // Create the buffer
            context->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                sceneUBOs[i].buffer.buffer,
                sceneUBOs[i].buffer.memory);

            // Setup descriptor buffer info
            sceneUBOs[i].buffer.descriptor.buffer = sceneUBOs[i].buffer.buffer;
            sceneUBOs[i].buffer.descriptor.offset = 0;
            sceneUBOs[i].buffer.descriptor.range = bufferSize;

            // Map the memory
            VK_CHECK_RESULT(vkMapMemory(context->getDevice(), sceneUBOs[i].buffer.memory,
                0, bufferSize, 0, &sceneUBOs[i].buffer.mapped));

            // Allocate descriptor set
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = scenePool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &sceneLayout;

            VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &sceneUBOs[i].buffer.descriptorSet));

            // Write the UBO buffer info to the descriptor set
            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = sceneUBOs[i].buffer.descriptorSet;
            writeDescriptorSet.dstBinding = 0;  // Binding 0 in Set 0
            writeDescriptorSet.pBufferInfo = &sceneUBOs[i].buffer.descriptor;

            vkUpdateDescriptorSets(context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    void DescriptorManager::updateSceneUBO(uint32_t cameraIndex, const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos)
    {
        if (cameraIndex >= sceneUBOs.size()) {
            return;
        }

        sceneUBOs[cameraIndex].uniformBlock.projection = projection;
        sceneUBOs[cameraIndex].uniformBlock.view = view;
        sceneUBOs[cameraIndex].uniformBlock.viewProj = projection * view;
        sceneUBOs[cameraIndex].uniformBlock.cameraPos = cameraPos;

        VkDeviceSize bufferSize = sizeof(SceneUBO::UniformBlock);

        memcpy(sceneUBOs[cameraIndex].buffer.mapped, &sceneUBOs[cameraIndex].uniformBlock, bufferSize);
    }


    void DescriptorManager::createLightsData()
    {
        VkDeviceSize bufferSize = sizeof(LightsInfoUBO::UniformBlock);

        // Create the buffer
        context->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            lightInfoUBO.buffer.buffer,
            lightInfoUBO.buffer.memory);

        // Setup descriptor buffer info
        lightInfoUBO.buffer.descriptor.buffer = lightInfoUBO.buffer.buffer;
        lightInfoUBO.buffer.descriptor.offset = 0;
        lightInfoUBO.buffer.descriptor.range = bufferSize;

        // Map the memory
        VK_CHECK_RESULT(vkMapMemory(context->getDevice(), lightInfoUBO.buffer.memory,
            0, bufferSize, 0, &lightInfoUBO.buffer.mapped));

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = scenePool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &sceneLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &lightInfoUBO.buffer.descriptorSet));


        bufferSize = maxDirectionalLights * sizeof(DirectionalLightBufferData);

        context->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            directionalLightSSBO.buffer.buffer,
            directionalLightSSBO.buffer.memory);

        directionalLightSSBO.buffer.descriptor.buffer = directionalLightSSBO.buffer.buffer;
        directionalLightSSBO.buffer.descriptor.offset = 0;
        directionalLightSSBO.buffer.descriptor.range = bufferSize;

        VK_CHECK_RESULT(vkMapMemory(context->getDevice(), directionalLightSSBO.buffer.memory,
            0, bufferSize, 0, &directionalLightSSBO.buffer.mapped));

        // ... point lights setup ...
        bufferSize = maxPointLights * sizeof(PointLightBufferData);

        context->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            pointLightSSBO.buffer.buffer,
            pointLightSSBO.buffer.memory);

        pointLightSSBO.buffer.descriptor.buffer = pointLightSSBO.buffer.buffer;
        pointLightSSBO.buffer.descriptor.offset = 0;
        pointLightSSBO.buffer.descriptor.range = bufferSize;

        VK_CHECK_RESULT(vkMapMemory(context->getDevice(), pointLightSSBO.buffer.memory,
            0, bufferSize, 0, &pointLightSSBO.buffer.mapped));

        // ... spot lights setup ...
        bufferSize = maxSpotLights * sizeof(SpotLightBufferData);

        context->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            spotLightSSBO.buffer.buffer,
            spotLightSSBO.buffer.memory);

        spotLightSSBO.buffer.descriptor.buffer = spotLightSSBO.buffer.buffer;
        spotLightSSBO.buffer.descriptor.offset = 0;
        spotLightSSBO.buffer.descriptor.range = bufferSize;

        VK_CHECK_RESULT(vkMapMemory(context->getDevice(), spotLightSSBO.buffer.memory,
            0, bufferSize, 0, &spotLightSSBO.buffer.mapped));

        // Allocate a single descriptor set for all lights from the lights layout
        allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = scenePool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &lightsLayout;

        VkDescriptorSet lightsDescriptorSet;
        VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &lightsDescriptorSet));

        // Write lights buffers + shadow textures and sampler to the single lights descriptor set
        std::array<VkWriteDescriptorSet, 8> lightWrites{};

        // Light info UBO
        lightWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightWrites[0].descriptorCount = 1;
        lightWrites[0].dstSet = lightsDescriptorSet;
        lightWrites[0].dstBinding = 0;  // Light info UBO at binding 0
        lightWrites[0].pBufferInfo = &lightInfoUBO.buffer.descriptor;

        // Directional lights SSBO
        lightWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lightWrites[1].descriptorCount = 1;
        lightWrites[1].dstSet = lightsDescriptorSet;
        lightWrites[1].dstBinding = 1;  // Directional lights at binding 1
        lightWrites[1].pBufferInfo = &directionalLightSSBO.buffer.descriptor;

        // Point lights SSBO
        lightWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lightWrites[2].descriptorCount = 1;
        lightWrites[2].dstSet = lightsDescriptorSet;
        lightWrites[2].dstBinding = 2;  // Point lights at binding 2
        lightWrites[2].pBufferInfo = &pointLightSSBO.buffer.descriptor;

        // Spot lights SSBO
        lightWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        lightWrites[3].descriptorCount = 1;
        lightWrites[3].dstSet = lightsDescriptorSet;
        lightWrites[3].dstBinding = 3;  // Spot lights at binding 3
        lightWrites[3].pBufferInfo = &spotLightSSBO.buffer.descriptor;

        // Directional shadow 2D array (binding 4)
        lightWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        lightWrites[4].descriptorCount = 1;
        lightWrites[4].dstSet = lightsDescriptorSet;
        lightWrites[4].dstBinding = 4;
        lightWrites[4].pImageInfo = &defaultImageInfo; // imageView/layout of 2D array

        // Point shadow cube array (binding 5)
        lightWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        lightWrites[5].descriptorCount = 1;
        lightWrites[5].dstSet = lightsDescriptorSet;
        lightWrites[5].dstBinding = 5;
        lightWrites[5].pImageInfo = &cubeImageInfo; // cube image view + layout

        // Spot shadow 2D array (binding 6)
        lightWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        lightWrites[6].descriptorCount = 1;
        lightWrites[6].dstSet = lightsDescriptorSet;
        lightWrites[6].dstBinding = 6;
        lightWrites[6].pImageInfo = &defaultImageInfo;

        // Shadow sampler (binding 7)
        VkDescriptorImageInfo shadowSamplerInfo{};
        shadowSamplerInfo.sampler = defaultSampler; // use manager's sampler
        lightWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        lightWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        lightWrites[7].descriptorCount = 1;
        lightWrites[7].dstSet = lightsDescriptorSet;
        lightWrites[7].dstBinding = 7;
        lightWrites[7].pImageInfo = &shadowSamplerInfo;

        vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(lightWrites.size()), lightWrites.data(), 0, nullptr);
        // Store the descriptor set in each SSBO (optional, for convenience)
        directionalLightSSBO.descriptorSet = lightsDescriptorSet;
        pointLightSSBO.descriptorSet = lightsDescriptorSet;
        spotLightSSBO.descriptorSet = lightsDescriptorSet;
        shadowMapArray.descriptorSet = lightsDescriptorSet;
        cubeMapShadowMapArray.descriptorSet = lightsDescriptorSet;
    }

    void DescriptorManager::updateLightsData(const std::vector<DirectionalLightBufferData>& directionalLights,
        const std::vector<PointLightBufferData>& pointLights, const std::vector<SpotLightBufferData>& spotLights, float farPlane)
    {

        lightInfoUBO.uniformBlock.directionalLightCount = directionalLights.size();
        lightInfoUBO.uniformBlock.pointLightCount = pointLights.size();
        lightInfoUBO.uniformBlock.spotLightCount = spotLights.size();
        lightInfoUBO.uniformBlock.far_plane = farPlane;

        VkDeviceSize bufferSize = sizeof(LightsInfoUBO::UniformBlock);
        memcpy(lightInfoUBO.buffer.mapped, &lightInfoUBO.uniformBlock, bufferSize);

        // Update directional lights
        if (!directionalLights.empty()) {
            size_t copySize = std::min(static_cast<size_t>(maxDirectionalLights), directionalLights.size());
            memcpy(directionalLightSSBO.buffer.mapped,
                   directionalLights.data(),
                   copySize * sizeof(DirectionalLightBufferData));
        }

        // Update point lights
        if (!pointLights.empty()) {
            size_t copySize = std::min(static_cast<size_t>(maxPointLights), pointLights.size());
            memcpy(pointLightSSBO.buffer.mapped,
                   pointLights.data(),
                   copySize * sizeof(PointLightBufferData));
        }

        // Update spot lights
        if (!spotLights.empty()) {
            size_t copySize = std::min(static_cast<size_t>(maxSpotLights), spotLights.size());
            memcpy(spotLightSSBO.buffer.mapped,
                   spotLights.data(),
                   copySize * sizeof(SpotLightBufferData));
        }
    }

    void DescriptorManager::updateShadowDescriptorSet(VkImageView directionalView, VkImageView pointView, VkImageView spotView)
    {
        std::array<VkWriteDescriptorSet, 3> writeDescriptorSets;

        VkDescriptorImageInfo directionalImageInfo = {};
        directionalImageInfo.sampler = defaultSampler;
        directionalImageInfo.imageView = directionalView;
        directionalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeDescriptorSets[0] = {};
        writeDescriptorSets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[0].dstSet = lightInfoUBO.buffer.descriptorSet;
        writeDescriptorSets[0].descriptorCount = 1;
        writeDescriptorSets[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[0].pImageInfo = &directionalImageInfo;
        writeDescriptorSets[0].dstBinding = 4;

        VkDescriptorImageInfo pointImageInfo = {};
        pointImageInfo.sampler = cubeSampler;
        pointImageInfo.imageView = pointView;
        pointImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeDescriptorSets[1] = {};
        writeDescriptorSets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[1].dstSet = lightInfoUBO.buffer.descriptorSet;
        writeDescriptorSets[1].descriptorCount = 1;
        writeDescriptorSets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[1].pImageInfo = &pointImageInfo;
        writeDescriptorSets[1].dstBinding = 5;

        VkDescriptorImageInfo spotImageInfo = {};
        spotImageInfo.sampler = defaultSampler;
        spotImageInfo.imageView = spotView;
        spotImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        writeDescriptorSets[2] = {};
        writeDescriptorSets[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSets[2].dstSet = lightInfoUBO.buffer.descriptorSet;
        writeDescriptorSets[2].descriptorCount = 1;
        writeDescriptorSets[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSets[2].pImageInfo = &spotImageInfo;
        writeDescriptorSets[2].dstBinding = 6;

        vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    std::vector<VkDescriptorSetLayout> DescriptorManager::getLayoutsFromEnums(std::vector<ShaderDefinesEnum> definitions)
    {
        std::vector<VkDescriptorSetLayout> layouts;
        for (auto definition : definitions) {
            switch (definition)
            {
            case SCENE_UBO_GLSL:
                layouts.push_back(sceneLayout);
                break;
            case LIGHTING_COMMON_GLSL:
                layouts.push_back(lightsLayout);
                break;
            case VERTEX_IO_GLSL:
                layouts.push_back(meshUniformLayout);
                break;
            case MATERIAL_PBR_GLSL:
                layouts.push_back(pbrMaterialLayout);
                break;
            case MATERIAL_SKYBOX_GLSL:
                layouts.push_back(skyboxMaterialLayout);
                break;
            }
        }
        return layouts;
    }
} // namespace vks
