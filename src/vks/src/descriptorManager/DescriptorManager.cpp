
#include "DescriptorManager.h"
#include <stdexcept>

namespace vks {

DescriptorManager::DescriptorManager(am::AssetManagerInterface* assetManager, VulkanContext* context)
    : assetManager(assetManager)
    , context(context) {
}

DescriptorManager::~DescriptorManager() {
    cleanup();
}

void DescriptorManager::initialize() {
    createDescriptorPools();
    createDescriptorSetLayouts();
    createSceneUBO();
}

void DescriptorManager::cleanup() {
    auto device = context->getDevice();

    if (sceneUBO.buffer.buffer != VK_NULL_HANDLE) {
        if (sceneUBO.buffer.mapped) {
            vkUnmapMemory(device, sceneUBO.buffer.memory);
        }
        vkDestroyBuffer(device, sceneUBO.buffer.buffer, nullptr);
        vkFreeMemory(device, sceneUBO.buffer.memory, nullptr);
    }

    if (defaultSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, defaultSampler, nullptr);
        defaultSampler = VK_NULL_HANDLE;
    }

    // Clear resource cache
    loadedResources.clear();

    // Destroy descriptor sets layouts
    if (materialLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
    }
    if (meshUniformLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, meshUniformLayout, nullptr);
    }
    if (sceneLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, sceneLayout, nullptr);
    }

    // Destroy descriptor pools
    if (materialPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, materialPool, nullptr);
    }
    if (meshPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, meshPool, nullptr);
    }
    if (scenePool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, scenePool, nullptr);
    }
}

void DescriptorManager::createDescriptorPools() {
    // Material pool (no changes needed)
    std::vector<VkDescriptorPoolSize> materialPoolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000}
    };

    VkDescriptorPoolCreateInfo materialPoolInfo{};
    materialPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    materialPoolInfo.poolSizeCount = static_cast<uint32_t>(materialPoolSizes.size());
    materialPoolInfo.pPoolSizes = materialPoolSizes.data();
    materialPoolInfo.maxSets = 1000;

    if (vkCreateDescriptorPool(context->getDevice(), &materialPoolInfo, nullptr, &materialPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create material descriptor pool!");
    }

    // Mesh pool (no changes needed)
    VkDescriptorPoolSize meshPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000};
    VkDescriptorPoolCreateInfo meshPoolInfo{};
    meshPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    meshPoolInfo.poolSizeCount = 1;
    meshPoolInfo.pPoolSizes = &meshPoolSize;
    meshPoolInfo.maxSets = 1000;

    if (vkCreateDescriptorPool(context->getDevice(), &meshPoolInfo, nullptr, &meshPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create mesh descriptor pool!");
    }

    // Scene pool (updated to include skybox resources)
    std::vector<VkDescriptorPoolSize> scenePoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},           // For camera, lighting, and skybox uniforms
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}    // For environment maps and skybox cubemap
    };
    
    VkDescriptorPoolCreateInfo scenePoolInfo{};
    scenePoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    scenePoolInfo.poolSizeCount = static_cast<uint32_t>(scenePoolSizes.size());
    scenePoolInfo.pPoolSizes = scenePoolSizes.data();
    scenePoolInfo.maxSets = 100;
    
    if (vkCreateDescriptorPool(context->getDevice(), &scenePoolInfo, nullptr, &scenePool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create scene descriptor pool!");
    }
}

    void DescriptorManager::createDefaultSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.maxAnisotropy = 8.0f;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxLod = 32.0f; // Large enough for most mipmaps

    VK_CHECK_RESULT(vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &defaultSampler));

    // Setup default image info
    defaultImageInfo.sampler = defaultSampler;
    defaultImageInfo.imageView = VK_NULL_HANDLE;
    defaultImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}


void DescriptorManager::createDescriptorSetLayouts() {
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
        // Normal map texture binding
        /*{ For now we don't care
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        }*/
    };
    // Material layout

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount = static_cast<uint32_t>(materialBindings.size());
    materialLayoutInfo.pBindings = materialBindings.data();

    if (vkCreateDescriptorSetLayout(context->getDevice(), &materialLayoutInfo, nullptr, &materialLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create material descriptor set layout!");
    }

    // Mesh layout
    VkDescriptorSetLayoutBinding meshBinding{};
    meshBinding.binding = 0;
    meshBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    meshBinding.descriptorCount = 1;
    meshBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo meshLayoutInfo{};
    meshLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    meshLayoutInfo.bindingCount = 1;
    meshLayoutInfo.pBindings = &meshBinding;

    if (vkCreateDescriptorSetLayout(context->getDevice(), &meshLayoutInfo, nullptr, &meshUniformLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create mesh descriptor set layout!");
    }

    // Scene layout (updated to match skybox shader requirements)
    std::vector<VkDescriptorSetLayoutBinding> sceneBindings = {
        // UBO used by both vertex shaders (matrices)
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        // Cubemap sampler used by skybox fragment shader
        //{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };

    VkDescriptorSetLayoutCreateInfo sceneLayoutInfo{};
    sceneLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    sceneLayoutInfo.bindingCount = static_cast<uint32_t>(sceneBindings.size());
    sceneLayoutInfo.pBindings = sceneBindings.data();
    
    if (vkCreateDescriptorSetLayout(context->getDevice(), &sceneLayoutInfo, nullptr, &sceneLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create scene descriptor set layout!");
    }
}

std::vector<VkDescriptorSetLayout> DescriptorManager::getAllLayouts() const {
    return {sceneLayout, materialLayout, meshUniformLayout};
}

bool DescriptorManager::isResourceLoaded(const boost::uuids::uuid& assetId)
{
    return loadedResources.find(assetId) != loadedResources.end();
}

void DescriptorManager::createSceneUBO() {
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
    void DescriptorManager::updateSceneUBO(const glm::mat4& projection, const glm::mat4& view,
        const glm::mat4& model, const glm::vec3& lightPos) {
    sceneUBO.uniformBlock.projection = projection;
    sceneUBO.uniformBlock.view = view;
    sceneUBO.uniformBlock.model = model;
    sceneUBO.uniformBlock.normal = glm::transpose(glm::inverse(model));
    sceneUBO.uniformBlock.lightpos = glm::vec4(lightPos.x,lightPos.y,lightPos.z,0);

    VkDeviceSize bufferSize = sizeof(SceneUBO::UniformBlock);

    // Copy to GPU memory
    memcpy(sceneUBO.buffer.mapped, &sceneUBO.uniformBlock, bufferSize);
    // No barrier needed here - it will be handled in RenderManager
}
} // namespace vks