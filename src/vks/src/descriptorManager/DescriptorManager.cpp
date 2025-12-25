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
        createLightSSBO();
    }

    void DescriptorManager::cleanup()
    {
        auto device = context->getDevice();

        if (sceneUBO.buffer.buffer != VK_NULL_HANDLE)
        {
            if (sceneUBO.buffer.mapped)
            {
                vkUnmapMemory(device, sceneUBO.buffer.memory);
            }
            vkDestroyBuffer(device, sceneUBO.buffer.buffer, nullptr);
            vkFreeMemory(device, sceneUBO.buffer.memory, nullptr);
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

        if (defaultSampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, defaultSampler, nullptr);
            defaultSampler = VK_NULL_HANDLE;
        }

        // Clear resource cache
        loadedResources.clear();

        // Destroy descriptor sets layouts
        if (materialLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
        }
        if (meshUniformLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, meshUniformLayout, nullptr);
        }
        if (sceneLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, sceneLayout, nullptr);
        }

        // Destroy descriptor pools
        if (materialPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, materialPool, nullptr);
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

        if (vkCreateDescriptorPool(context->getDevice(), &materialPoolInfo, nullptr, &materialPool) != VK_SUCCESS)
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

        // Scene pool (updated to include skybox resources)
        std::vector<VkDescriptorPoolSize> scenePoolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100}, // For camera, lighting, and skybox uniforms
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}, // For environment maps and skybox cubemap
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100}
        };

        VkDescriptorPoolCreateInfo scenePoolInfo{};
        scenePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        scenePoolInfo.poolSizeCount = static_cast<uint32_t>(scenePoolSizes.size());
        scenePoolInfo.pPoolSizes = scenePoolSizes.data();
        scenePoolInfo.maxSets = 100;

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

        createDefaultTexture();

        // Setup default image info
        defaultImageInfo.sampler = defaultSampler;
        defaultImageInfo.imageView = defaultImageView;
        defaultImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }


void DescriptorManager::createDescriptorSetLayouts()
{
    // Set 0: Mesh/Vertex shader uniforms (UNCHANGED)
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

    // Set 1: Material descriptors (sampler + images)
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,  // CHANGED to FRAGMENT_BIT
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

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount = static_cast<uint32_t>(materialBindings.size());
    materialLayoutInfo.pBindings = materialBindings.data();

    if (vkCreateDescriptorSetLayout(context->getDevice(), &materialLayoutInfo, nullptr, &materialLayout) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("failed to create material descriptor set layout!");
    }

    // Set 2: Scene descriptors (UNCHANGED)
    std::vector<VkDescriptorSetLayoutBinding> sceneBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };

    VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
    sceneLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneLayoutInfo.bindingCount = static_cast<uint32_t>(sceneBindings.size());
    sceneLayoutInfo.pBindings = sceneBindings.data();

    if (vkCreateDescriptorSetLayout(context->getDevice(), &sceneLayoutInfo, nullptr, &sceneLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create scene descriptor set layout!");
    }
}
    std::vector<VkDescriptorSetLayout> DescriptorManager::getAllLayouts() const
    {
        return {meshUniformLayout, materialLayout, sceneLayout};  // Order matters!
    }

    bool DescriptorManager::isResourceLoaded(const boost::uuids::uuid& assetId)
    {
        return loadedResources.find(assetId) != loadedResources.end();
    }

    void DescriptorManager::createSceneUBO()
    {
        VkDeviceSize bufferSize = sizeof(SceneUBO::UniformBlock);

        // Create the buffer
        context->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            sceneUBO.buffer.buffer,
            sceneUBO.buffer.memory);

        // Setup descriptor buffer info
        sceneUBO.buffer.descriptor.buffer = sceneUBO.buffer.buffer;
        sceneUBO.buffer.descriptor.offset = 0;
        sceneUBO.buffer.descriptor.range = bufferSize;

        // Map the memory
        VK_CHECK_RESULT(vkMapMemory(context->getDevice(), sceneUBO.buffer.memory,
            0, bufferSize, 0, &sceneUBO.buffer.mapped));

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = scenePool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &sceneLayout;

        VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &sceneUBO.buffer.descriptorSet));

        // Update descriptor set
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = sceneUBO.buffer.descriptorSet;
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.pBufferInfo = &sceneUBO.buffer.descriptor;

        vkUpdateDescriptorSets(context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);
    }

    void DescriptorManager::updateSceneUBO(const glm::mat4& projection, const glm::mat4& view, const glm::vec3 cameraPos)
    {
        sceneUBO.uniformBlock.projection = projection;
        sceneUBO.uniformBlock.view = view;
        sceneUBO.uniformBlock.viewProj = projection * view;
        sceneUBO.uniformBlock.cameraPos = cameraPos;

        VkDeviceSize bufferSize = sizeof(SceneUBO::UniformBlock);

        memcpy(sceneUBO.buffer.mapped, &sceneUBO.uniformBlock, bufferSize);
    }


void DescriptorManager::createLightSSBO()
{
    VkDeviceSize bufferSize = maxDirectionalLights * sizeof(DirectionalLightBufferData);

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

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = scenePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &sceneLayout;

    VK_CHECK_RESULT(
        vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &directionalLightSSBO.descriptorSet));

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = directionalLightSSBO.descriptorSet;
    writeDescriptorSet.dstBinding = 1;
    writeDescriptorSet.pBufferInfo = &directionalLightSSBO.buffer.descriptor;

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);

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

    allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = scenePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &sceneLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &pointLightSSBO.descriptorSet));

    writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = pointLightSSBO.descriptorSet;
    writeDescriptorSet.dstBinding = 2;
    writeDescriptorSet.pBufferInfo = &pointLightSSBO.buffer.descriptor;

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);

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

    allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = scenePool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &sceneLayout;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(context->getDevice(), &allocInfo, &spotLightSSBO.descriptorSet));

    writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.dstSet = spotLightSSBO.descriptorSet;
    writeDescriptorSet.dstBinding = 3;
    writeDescriptorSet.pBufferInfo = &spotLightSSBO.buffer.descriptor;

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);

    // NOW update the main scene UBO descriptor set with all three light SSBOs
    std::array<VkWriteDescriptorSet, 3> lightWrites{};

    lightWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightWrites[0].descriptorCount = 1;
    lightWrites[0].dstSet = sceneUBO.buffer.descriptorSet;
    lightWrites[0].dstBinding = 1;  // Directional lights at binding 1
    lightWrites[0].pBufferInfo = &directionalLightSSBO.buffer.descriptor;

    lightWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightWrites[1].descriptorCount = 1;
    lightWrites[1].dstSet = sceneUBO.buffer.descriptorSet;
    lightWrites[1].dstBinding = 2;  // Point lights at binding 2
    lightWrites[1].pBufferInfo = &pointLightSSBO.buffer.descriptor;

    lightWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    lightWrites[2].descriptorCount = 1;
    lightWrites[2].dstSet = sceneUBO.buffer.descriptorSet;
    lightWrites[2].dstBinding = 3;  // Spot lights at binding 3
    lightWrites[2].pBufferInfo = &spotLightSSBO.buffer.descriptor;

    vkUpdateDescriptorSets(context->getDevice(), 3, lightWrites.data(), 0, nullptr);
}
    void DescriptorManager::updateLightSSBO(const std::vector<DirectionalLightBufferData>& directionalLights,
        const std::vector<PointLightBufferData>& pointLights, const std::vector<SpotLightBufferData>& spotLights)
    {
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

} // namespace vks
