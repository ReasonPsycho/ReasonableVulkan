//
// Created by redkc on 06/08/2025.
//

#ifndef VULKANAMMODEL_H
#define VULKANAMMODEL_H

#include <stdlib.h>
#include <string>
#include <fstream>
#include <vector>

#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#include <ktx.h>
#include <ktxvulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vkAm
{
	enum DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,
		ImageNormalMap = 0x00000002
	};

	extern VkDescriptorSetLayout descriptorSetLayoutImage;
	extern VkDescriptorSetLayout descriptorSetLayoutUbo;
	extern VkMemoryPropertyFlags memoryPropertyFlags;
	extern uint32_t descriptorBindingFlags;

	struct Node;

    struct Texture {
		vks::VulkanDevice *device = nullptr;
		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory deviceMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;
		uint32_t index;

		void updateDescriptor();

		void destroy();

		void fromglTfImage(tinygltf::Image &gltfimage, std::string path, vks::VulkanDevice *device, VkQueue copyQueue);
	};

	struct Material {
		vks::VulkanDevice *device = nullptr;

		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };

		AlphaMode alphaMode = ALPHAMODE_OPAQUE;
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		vkglTF::Texture *baseColorTexture = nullptr;
		vkglTF::Texture *metallicRoughnessTexture = nullptr;
		vkglTF::Texture *normalTexture = nullptr;
		vkglTF::Texture *occlusionTexture = nullptr;
		vkglTF::Texture *emissiveTexture = nullptr;

		vkglTF::Texture *specularGlossinessTexture;
		vkglTF::Texture *diffuseTexture;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		Material(vks::VulkanDevice *device) : device(device) {
		};

		void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
		                         uint32_t descriptorBindingFlags);
	};

	struct Mesh {
		vks::VulkanDevice *device;
		std::string name;

		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t firstVertex;
		uint32_t vertexCount;
		Material &material;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

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

		Mesh(vks::VulkanDevice *device, glm::mat4 matrix);

		~Mesh();
	};

	struct Node {
		Node *parent;
		uint32_t index;
		std::vector<Node *> children;
		glm::mat4 matrix;
		std::string name;
		Mesh *mesh;
		int32_t skinIndex = -1;
		glm::vec3 translation{};
		glm::vec3 scale{1.0f};
		glm::quat rotation{};

		glm::mat4 localMatrix();

		glm::mat4 getMatrix();

		void update();

		~Node();
	};

	enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;
		glm::vec4 joint0;
		glm::vec4 weight0;
		glm::vec4 tangent;
		static VkVertexInputBindingDescription vertexInputBindingDescription;
		static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;

		static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);

		static VkVertexInputAttributeDescription inputAttributeDescription(
			uint32_t binding, uint32_t location, VertexComponent component);

		static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(
			uint32_t binding, const std::vector<VertexComponent> components);

		/** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
		static VkPipelineVertexInputStateCreateInfo *getPipelineVertexInputState(
			const std::vector<VertexComponent> components);
	};

	enum FileLoadingFlags {
		None = 0x00000000,
		PreTransformVertices = 0x00000001,
		PreMultiplyVertexColors = 0x00000002,
		FlipY = 0x00000004,
		DontLoadImages = 0x00000008
	};

	enum RenderFlags {
		BindImages = 0x00000001,
		RenderOpaqueNodes = 0x00000002,
		RenderAlphaMaskedNodes = 0x00000004,
		RenderAlphaBlendedNodes = 0x00000008
	};

		class Model {
	private:
		vkglTF::Texture *getTexture(uint32_t index);

		vkglTF::Texture emptyTexture;

		void createEmptyTexture(VkQueue transferQueue);

	public:
		vks::VulkanDevice *device;
		VkDescriptorPool descriptorPool;

		struct Vertices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} vertices;

		struct Indices {
			int count;
			VkBuffer buffer;
			VkDeviceMemory memory;
		} indices;

		std::vector<Node *> nodes;
		std::vector<Node *> linearNodes;

		std::vector<Texture> textures;
		std::vector<Material> materials;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		bool metallicRoughnessWorkflow = true;
		bool buffersBound = false;
		std::string path;

		Model() {
		};

		~Model();
			//TODO this should be a model
			void loadFromFile(std::string filename, vks::VulkanDevice *device, VkQueue transferQueue,
							  uint32_t fileLoadingFlags = vkglTF::FileLoadingFlags::None, float scale = 1.0f);

			//TODO those two function should just rebind my am model
		void loadNode(vkglTF::Node *parent, const tinygltf::Node &node, uint32_t nodeIndex,
		              const tinygltf::Model &model, std::vector<uint32_t> &indexBuffer,
		              std::vector<Vertex> &vertexBuffer, float globalscale);

		void loadMaterials(tinygltf::Model &gltfModel);


		void bindBuffers(VkCommandBuffer commandBuffer);

		void drawNode(Node *node, VkCommandBuffer commandBuffer, uint32_t renderFlags = 0,
		              VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);

		void draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0,
		          VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);

		void getNodeDimensions(Node *node, glm::vec3 &min, glm::vec3 &max);

		void getSceneDimensions();

		Node *findNode(Node *parent, uint32_t index);

		Node *nodeFromIndex(uint32_t index);

		void prepareNodeDescriptor(vkglTF::Node *node, VkDescriptorSetLayout descriptorSetLayout);
	};
}




#endif //VULKANAMMODEL_H
