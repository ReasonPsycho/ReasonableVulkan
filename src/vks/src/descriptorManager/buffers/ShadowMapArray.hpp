//
// Created by redkc on 22/03/2026.
//

#ifndef REASONABLEVULKAN_SHADOWMAPARRAY_HPP
#define REASONABLEVULKAN_SHADOWMAPARRAY_HPP


struct ShadowMapArray
{
    struct Buffer
    {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDescriptorBufferInfo descriptor;
        void* mapped;
    } buffer;

    VkDescriptorSet descriptorSet;
};


#endif //REASONABLEVULKAN_SHADOWMAPARRAY_HPP
