//
// Created by redkc on 10/08/2025.
//

#include "VertexHandle.h"

namespace vks {

	VkVertexInputBindingDescription VertexHandle::vertexInputBindingDescription;
	std::vector<VkVertexInputAttributeDescription> VertexHandle::vertexInputAttributeDescriptions;
	VkPipelineVertexInputStateCreateInfo VertexHandle::pipelineVertexInputStateCreateInfo;

VkVertexInputBindingDescription VertexHandle::inputBindingDescription(uint32_t binding) {
	return VkVertexInputBindingDescription({binding, sizeof(VertexHandle), VK_VERTEX_INPUT_RATE_VERTEX});
}

VkVertexInputAttributeDescription VertexHandle::inputAttributeDescription(
	uint32_t binding, uint32_t location, VertexComponent component) {
	switch (component) {
		case VertexComponent::Position:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexHandle, vertex.Position)
			});
		case VertexComponent::Normal:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexHandle, vertex.Normal)
			});
		case VertexComponent::UV:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexHandle, vertex.TexCoords)
			});
		case VertexComponent::Color:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexHandle, vertex.Color)
			});
		case VertexComponent::Tangent:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexHandle, vertex.Tangent)
			});
		case VertexComponent::Bitangent:
			return VkVertexInputAttributeDescription({
				location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexHandle, vertex.Bitangent)
			});
		default:
			return VkVertexInputAttributeDescription({});
	}
}

std::vector<VkVertexInputAttributeDescription> VertexHandle::inputAttributeDescriptions(
	uint32_t binding, const std::vector<VertexComponent> components) {
	std::vector<VkVertexInputAttributeDescription> result;
	uint32_t location = 0;
	for (VertexComponent component: components) {
		result.push_back(VertexHandle::inputAttributeDescription(binding, location, component));
		location++;
	}
	return result;
}
    
} // vks