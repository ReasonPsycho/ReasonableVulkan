
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
}

void DescriptorManager::cleanup() {
    auto device = context->getDevice();
    
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

void DescriptorManager::createDescriptorSetLayouts() {

    // Material layout
    std::vector<VkDescriptorSetLayoutBinding> materialBindings = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    
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
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
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

} // namespace vks