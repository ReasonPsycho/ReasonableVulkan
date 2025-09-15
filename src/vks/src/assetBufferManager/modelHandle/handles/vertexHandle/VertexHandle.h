//
// Created by redkc on 10/08/2025.
//

#ifndef VERTEX_H
#define VERTEX_H
#include <vector>
#include <vulkan/vulkan_core.h>

#include "VertexHandle.hpp"
#include "../IVulkanHandle.h"

namespace vks {

    enum class VertexComponent { Position, Normal, UV, Color, Tangent, Bitangent};

    struct VertexHandle : public IVulkanHandle
    {
        am::VertexHandle vertex;
        static VkVertexInputBindingDescription vertexInputBindingDescription;
        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

        static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);

        static VkVertexInputAttributeDescription inputAttributeDescription(
            uint32_t binding, uint32_t location, VertexComponent component);

        static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(
            uint32_t binding, const std::vector<VertexComponent> components);

        /** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
        static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(
            const std::vector<VertexComponent> components);
        void cleanup() override {};

    };

} // vks

#endif //VERTEX_H
