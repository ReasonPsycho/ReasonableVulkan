//
// Created by redkc on 10/09/2025.
//

#ifndef SHADERHANDLE_H
#define SHADERHANDLE_H
#include "../IVulkanHandle.h"
#include "assetDatas/ShaderData.h"


class ShaderHandle : public vks::IVulkanHandle{

public:
    ShaderHandle(am::ShaderData& textureData, vks::base::VulkanDevice* device, VkQueue copyQueue) : IVulkanHandle(device,copyQueue)
    {};

    void cleanup() override {};

};



#endif //SHADERHANDLE_H
