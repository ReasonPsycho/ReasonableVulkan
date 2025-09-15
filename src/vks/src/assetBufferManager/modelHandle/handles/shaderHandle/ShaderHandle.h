//
// Created by redkc on 10/09/2025.
//

#ifndef SHADERHANDLE_H
#define SHADERHANDLE_H
#include "../IVulkanHandle.h"
#include "assetDatas/ShaderData.h"


class ShaderHandle : vks::IVulkanHandle{


    ShaderHandle(am::ShaderData& textureData, vks::base::VulkanDevice* device, VkQueue copyQueue);

    void cleanup() override {};

};



#endif //SHADERHANDLE_H
