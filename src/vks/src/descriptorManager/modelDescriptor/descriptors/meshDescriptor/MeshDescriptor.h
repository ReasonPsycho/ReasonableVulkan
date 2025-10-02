
//
// Created by redkc on 10/08/2025.
//

#ifndef MESH_HANDLE_H
#define MESH_HANDLE_H
#include <string>
#include <glm/fwd.hpp>

#include "../materialDescriptor/MaterialDescriptor.h"
#include "../IVulkanDescriptor.h"
#include "glm/glm.hpp"
#include "assetDatas/MeshData.h"

namespace am
{
    class MaterialAsset;
}


namespace vks
{

    class DescriptorManager;

    // Move vertex component enum to MeshHandle
    enum class VertexComponent { Position, Normal, UV, Color, Tangent, Bitangent};

    class MeshDescriptor : public IVulkanDescriptor {
        std::string name;
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t firstVertex;
        uint32_t vertexCount;

        // Vertex input description - moved from VertexHandle
        static VkVertexInputBindingDescription vertexInputBindingDescription;
        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
        static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

    public:
        MeshDescriptor(DescriptorManager* assetHandleManager,am::MeshData& meshData, glm::mat4 matrix,VulkanContext& vulkanContext);
        ~MeshDescriptor();
        void setUpDescriptorSet(VkDescriptorSetLayout meshUniformLayout,VkDescriptorPool meshDescriptorPool);
        void cleanup() override {};

        static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
        static VkVertexInputAttributeDescription inputAttributeDescription(
            uint32_t binding, uint32_t location, VertexComponent component);
        static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(
            uint32_t binding, const std::vector<VertexComponent> components);
        static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(
            const std::vector<VertexComponent> components);

        MaterialDescriptor* material;

        struct UniformBuffer {
            vks::base::Buffer buffer;
            VkDescriptorBufferInfo descriptor;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
            void *mapped;
        } uniformBuffer;

        struct UniformBlock {
            glm::mat4 matrix;
            glm::mat4 jointMatrix[64]{};
            float jointcount{0};
        } uniformBlock;

        struct Vertices
        {
            int count;
            vks::base::Buffer buffer;
        } vertices{};

        struct Indices
        {
            int count;
            vks::base::Buffer buffer;
        } indices{};

    };
}


#endif //MESH_H