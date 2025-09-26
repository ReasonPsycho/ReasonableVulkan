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
#include "descriptors/materialDescriptor/MaterialDescriptor.h"
#include "ModelHandleNode.h"
namespace am
{
    class TextureAsset;
}

namespace vks
{
    class DescriptorManager;

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

    class ModelDescriptor : public IVulkanDescriptor
    {

    public:
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        std::vector<MeshDescriptor*> meshes;
        std::vector<NodeHandle*> nodes;
        std::vector<TextureDescriptor*> textures;
        std::vector<MaterialDescriptor*> materials;
        bool metallicRoughnessWorkflow = true;

        ModelDescriptor(DescriptorManager* assetHandleManager,am::ModelData modelData,vks::base::VulkanDevice* device, VkQueue* transferQueue);

        ~ModelDescriptor();

        //TODO those two function should just rebind my am model
        void loadNode(DescriptorManager* assetHandleManager,NodeHandle* parent, const am::Node& node,
                      vks::ModelDescriptor& model);

        void cleanup() override{};
    };
}


#endif //VULKANAMMODEL_H
