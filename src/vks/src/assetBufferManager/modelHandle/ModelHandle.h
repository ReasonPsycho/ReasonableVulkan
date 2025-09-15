//
// Created by redkc on 06/08/2025.
//

#ifndef VULKANAMMODEL_H
#define VULKANAMMODEL_H

#include <string>
#include <fstream>
#include <vector>
#include <ktx.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "assetDatas/MaterialData.h"
#include "assetDatas/ModelData.h"
#include "handles/materialHandle/MaterialHandle.h"
#include "handles/nodeHandle/NodeHandle.h"
namespace am
{
    class TextureAsset;
}

namespace vks
{
    class AssetHandleManager;

    extern VkDescriptorSetLayout descriptorSetLayoutImage;
    extern VkDescriptorSetLayout descriptorSetLayoutUbo;
    extern VkMemoryPropertyFlags memoryPropertyFlags;
    extern uint32_t descriptorBindingFlags;


    enum FileLoadingFlags
    {
        None = 0x00000000,
        PreTransformVertices = 0x00000001,
        PreMultiplyVertexColors = 0x00000002,
        FlipY = 0x00000004,
        DontLoadImages = 0x00000008
    };

    enum RenderFlags
    {
        BindImages = 0x00000001,
        RenderOpaqueNodes = 0x00000002,
        RenderAlphaMaskedNodes = 0x00000004,
        RenderAlphaBlendedNodes = 0x00000008
    };

    class ModelHandle : public IVulkanHandle
    {

    public:
        vks::base::VulkanDevice* device;
        VkDescriptorPool descriptorPool;

        std::vector<MeshHandle*> meshes;
        std::vector<NodeHandle*> nodes;
        std::vector<TextureHandle*> textures;
        std::vector<MaterialHandle*> materials;
        bool metallicRoughnessWorkflow = true;

        ModelHandle(am::ModelData modelData,vks::base::VulkanDevice* device, VkQueue transferQueue);

        ~ModelHandle();

        //TODO those two function should just rebind my am model
        void loadNode(AssetHandleManager assetHandleManager,NodeHandle* parent, const am::Node& node,
                      const vks::ModelHandle& model, std::vector<uint32_t>& indexBuffer,
                      std::vector<am::VertexHandle>& vertexBuffer);

        void cleanup() override{};
    };
}


#endif //VULKANAMMODEL_H
