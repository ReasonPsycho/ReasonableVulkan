#include <boost/test/unit_test.hpp>
#include "../src/base/VulkanTools.h"
#include "../src/base/VulkanInitializers.hpp"
#include "../src/base/VulkanBuffer.h"

BOOST_AUTO_TEST_SUITE(VksHelpersTestSuite)

BOOST_AUTO_TEST_CASE(VulkanToolsErrorStringTest) {
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_NOT_READY), "NOT_READY");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_TIMEOUT), "TIMEOUT");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_EVENT_SET), "EVENT_SET");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_EVENT_RESET), "EVENT_RESET");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_INCOMPLETE), "INCOMPLETE");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_OUT_OF_HOST_MEMORY), "ERROR_OUT_OF_HOST_MEMORY");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_OUT_OF_DEVICE_MEMORY), "ERROR_OUT_OF_DEVICE_MEMORY");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_INITIALIZATION_FAILED), "ERROR_INITIALIZATION_FAILED");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_DEVICE_LOST), "ERROR_DEVICE_LOST");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_MEMORY_MAP_FAILED), "ERROR_MEMORY_MAP_FAILED");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_LAYER_NOT_PRESENT), "ERROR_LAYER_NOT_PRESENT");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_EXTENSION_NOT_PRESENT), "ERROR_EXTENSION_NOT_PRESENT");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_FEATURE_NOT_PRESENT), "ERROR_FEATURE_NOT_PRESENT");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_INCOMPATIBLE_DRIVER), "ERROR_INCOMPATIBLE_DRIVER");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_TOO_MANY_OBJECTS), "ERROR_TOO_MANY_OBJECTS");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_FORMAT_NOT_SUPPORTED), "ERROR_FORMAT_NOT_SUPPORTED");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_SURFACE_LOST_KHR), "ERROR_SURFACE_LOST_KHR");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR), "ERROR_NATIVE_WINDOW_IN_USE_KHR");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_SUBOPTIMAL_KHR), "SUBOPTIMAL_KHR");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(VK_ERROR_OUT_OF_DATE_KHR), "ERROR_OUT_OF_DATE_KHR");
    BOOST_CHECK_EQUAL(vks::base::tools::errorString(static_cast<VkResult>(99999)), "UNKNOWN_ERROR");
}

BOOST_AUTO_TEST_CASE(VulkanToolsPhysicalDeviceTypeStringTest) {
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(VK_PHYSICAL_DEVICE_TYPE_OTHER), "OTHER");
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU), "INTEGRATED_GPU");
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU), "DISCRETE_GPU");
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU), "VIRTUAL_GPU");
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(VK_PHYSICAL_DEVICE_TYPE_CPU), "CPU");
    BOOST_CHECK_EQUAL(vks::base::tools::physicalDeviceTypeString(static_cast<VkPhysicalDeviceType>(9999)), "UNKNOWN_DEVICE_TYPE");
}

BOOST_AUTO_TEST_CASE(VulkanToolsFormatAndAlignmentTest) {
    BOOST_CHECK(vks::base::tools::formatHasStencil(VK_FORMAT_D32_SFLOAT_S8_UINT));
    BOOST_CHECK(vks::base::tools::formatHasStencil(VK_FORMAT_D24_UNORM_S8_UINT));
    BOOST_CHECK(vks::base::tools::formatHasStencil(VK_FORMAT_D16_UNORM_S8_UINT));
    BOOST_CHECK(vks::base::tools::formatHasStencil(VK_FORMAT_S8_UINT));
    BOOST_CHECK(!vks::base::tools::formatHasStencil(VK_FORMAT_D32_SFLOAT));
    BOOST_CHECK(!vks::base::tools::formatHasStencil(VK_FORMAT_R8G8B8A8_UNORM));

    BOOST_CHECK_EQUAL(vks::base::tools::alignedSize(10, 16), 16);
    BOOST_CHECK_EQUAL(vks::base::tools::alignedSize(16, 16), 16);
    BOOST_CHECK_EQUAL(vks::base::tools::alignedSize(17, 16), 32);

    BOOST_CHECK_EQUAL(vks::base::tools::alignedVkSize(100, 64), 128);
    BOOST_CHECK_EQUAL(vks::base::tools::alignedVkSize(64, 64), 64);
    BOOST_CHECK_EQUAL(vks::base::tools::alignedVkSize(0, 64), 0);

    BOOST_CHECK(vks::base::tools::fileExists("res/textures/Missing_Texture.png"));
    BOOST_CHECK(!vks::base::tools::fileExists("res/textures/NonExistentTexture_123.png"));
}

BOOST_AUTO_TEST_CASE(VulkanInitializersHelperTest) {
    auto memAlloc = vks::base::initializers::memoryAllocateInfo();
    BOOST_CHECK_EQUAL(memAlloc.sType, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);

    auto bufInfo = vks::base::initializers::bufferCreateInfo(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1024);
    BOOST_CHECK_EQUAL(bufInfo.sType, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
    BOOST_CHECK_EQUAL(bufInfo.size, 1024);
    BOOST_CHECK_EQUAL(bufInfo.usage, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    auto imgInfo = vks::base::initializers::imageCreateInfo();
    BOOST_CHECK_EQUAL(imgInfo.sType, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

    auto samplerInfo = vks::base::initializers::samplerCreateInfo();
    BOOST_CHECK_EQUAL(samplerInfo.sType, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);

    auto descLayoutBinding = vks::base::initializers::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);
    BOOST_CHECK_EQUAL(descLayoutBinding.descriptorType, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    BOOST_CHECK_EQUAL(descLayoutBinding.stageFlags, VK_SHADER_STAGE_VERTEX_BIT);
    BOOST_CHECK_EQUAL(descLayoutBinding.binding, 0);

    auto poolSize = vks::base::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10);
    BOOST_CHECK_EQUAL(poolSize.type, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    BOOST_CHECK_EQUAL(poolSize.descriptorCount, 10);

    auto rasterState = vks::base::initializers::pipelineRasterizationStateCreateInfo(
        VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    BOOST_CHECK_EQUAL(rasterState.polygonMode, VK_POLYGON_MODE_FILL);
    BOOST_CHECK_EQUAL(rasterState.cullMode, VK_CULL_MODE_BACK_BIT);
    BOOST_CHECK_EQUAL(rasterState.frontFace, VK_FRONT_FACE_CLOCKWISE);

    auto blendAttachment = vks::base::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
    BOOST_CHECK_EQUAL(blendAttachment.colorWriteMask, 0xf);
    BOOST_CHECK_EQUAL(blendAttachment.blendEnable, VK_FALSE);

    auto depthStencil = vks::base::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    BOOST_CHECK_EQUAL(depthStencil.depthTestEnable, VK_TRUE);
    BOOST_CHECK_EQUAL(depthStencil.depthWriteEnable, VK_TRUE);
    BOOST_CHECK_EQUAL(depthStencil.depthCompareOp, VK_COMPARE_OP_LESS_OR_EQUAL);
}

BOOST_AUTO_TEST_CASE(VulkanBufferDescriptorSetupTest) {
    vks::base::Buffer buf;
    buf.buffer = reinterpret_cast<VkBuffer>(0x1234);
    buf.size = 2048;
    buf.setupDescriptor(1024, 64);
    BOOST_CHECK_EQUAL(buf.descriptor.buffer, reinterpret_cast<VkBuffer>(0x1234));
    BOOST_CHECK_EQUAL(buf.descriptor.range, 1024);
    BOOST_CHECK_EQUAL(buf.descriptor.offset, 64);

    buf.setupDescriptor();
    BOOST_CHECK_EQUAL(buf.descriptor.range, VK_WHOLE_SIZE);
    BOOST_CHECK_EQUAL(buf.descriptor.offset, 0);
}

BOOST_AUTO_TEST_SUITE_END()
