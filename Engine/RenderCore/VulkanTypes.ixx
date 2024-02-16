module;
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

export module RenderCore.VulkanTypes;

import RenderCore.VulkanDescriptors;

import <vector>;
import <functional>;
import <deque>;

export 
struct DeletionQueue
{
	std::deque<std::function<void()>> Deletors;

	void push_back(std::function<void()>&& function) {
		Deletors.push_back(function);
	}

	void flush() {
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = Deletors.rbegin(); it != Deletors.rend(); it++) {
			std::invoke(*it); // Call functors
		}

		Deletors.clear();
	}
};

export
struct FrameData
{
	VkSemaphore vkSwapchainSemaphore, vkRenderSemaphore;
	VkFence vkRenderFence;

	VkCommandPool vkCommandPool;
	VkCommandBuffer vkMainCommandBuffer;

	DeletionQueue vkDeletionQueue;
	DescriptorAllocatorGrowable vkFrameDescriptors;
};

export
struct AllocatedImage {
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};

export
struct ExtraPushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

// Holds all the data for a vertex. Overkill for simpler shaders.
export
struct Vertex {
	glm::vec3 position;
	float uv_x;
	glm::vec3 normal;
	float uv_y;
	glm::vec4 color;
	glm::vec3 tangent;
	float padding;
};

// Holds the loaded mesh vertex and index data
export
struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

// Holds the handles to allocated buffer
export
struct AllocatedBuffer {
	VkBuffer buffer;
	VmaAllocation allocation;
	VmaAllocationInfo info;
};

// Holds the resources needed for a mesh
export
struct GPUMeshBuffers {

	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress vertexBufferAddress;
};

export
struct VertexBufferPushConstants
{
	VkDeviceAddress vertexBuffer;
};

export
struct CameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

export typedef size_t MeshID;
export typedef size_t TextureID;
export typedef size_t ShaderID;
export typedef size_t RenderPipelineID;
