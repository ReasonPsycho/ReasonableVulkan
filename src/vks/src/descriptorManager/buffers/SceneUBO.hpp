//
// Created by redkc on 03/10/2025.
//

#ifndef REASONABLEVULKAN_SCENEUBO_HPP
#define REASONABLEVULKAN_SCENEUBO_HPP

#include <glm/glm.hpp>

// Add this to your class definition
struct SceneUBO {
    struct UniformBlock {
        alignas(16) glm::mat4 projection;
        alignas(16) glm::mat4 view;
    } uniformBlock;

    struct {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDescriptorSet descriptorSet;
        VkDescriptorBufferInfo descriptor;
        void* mapped = nullptr;
    } buffer;
};

#endif //REASONABLEVULKAN_SCENEUBO_HPP