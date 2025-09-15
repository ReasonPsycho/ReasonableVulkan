//
// Created by redkc on 10/08/2025.
//

#ifndef MESH_H
#define MESH_H
#include <string>
#include <glm/fwd.hpp>

#include "../materialHandle/MaterialHandle.h"
#include "../IVulkanHandle.h"
#include "glm/glm.hpp"
#include "assetDatas/MeshData.h"

namespace am
{
    class MaterialAsset;
}

namespace vks
{
    struct MeshHandle : IVulkanHandle {

        std::string name;
        MaterialHandle &material;
        uint32_t firstIndex;
        uint32_t indexCount;
        uint32_t firstVertex;
        uint32_t vertexCount;

        struct UniformBuffer {
            VkBuffer buffer;
            VkDeviceMemory memory;
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
            VkBuffer buffer;
            VkDeviceMemory memory;
        } vertices;

        struct Indices
        {
            int count;
            VkBuffer buffer;
            VkDeviceMemory memory;
        } indices;

        // Descriptor management
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        static VkDescriptorSetLayout descriptorSetLayoutUbo;

        MeshHandle(base::VulkanDevice *device, am::MeshData& meshData, glm::mat4 matrix, VkQueue copyQueue);

        ~MeshHandle();

        void createDescriptorPool();
        void createDescriptorSetLayout();
        void createDescriptorSet();

        void cleanup() override {};
    private:
        void setupDescriptors();
    };
}


#endif //MESH_H
