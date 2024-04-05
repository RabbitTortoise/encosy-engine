module;
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

export module RenderCore.VulkanTypes;

import <vector>;

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

// Holds the resources needed for a mesh
export
struct GPUModelMatrixBuffer {

	AllocatedBuffer matrixBuffer;
	VkDeviceAddress matrixBufferAddress;
};

export
struct VertexBufferPushConstants
{
	VkDeviceAddress vertexBuffer;
};

export
struct InstancedPushConstants
{
	VkDeviceAddress vertexBufferAddress;
	VkDeviceAddress modelMatrixBufferAddress;
};

export
struct CameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

export
struct LitLightingData
{
	glm::vec3 ambientLightColor;
	float ambientLightStrength;
	glm::vec3 directionalLightDir;
	float directionalLightStrength;
	glm::vec3 directionalLightColor;
};

export
struct LitLightingDataInShader
{
	glm::vec4 ambientLightColor;
	glm::vec4 directionalLightColor;
	glm::vec4 directionalLightDir;
};

export
struct TextureOptions
{
	glm::vec3 color;
	float textureRepeat;
};



export typedef size_t MeshID;
export typedef size_t TextureID;
export typedef size_t ShaderID;
export typedef size_t RenderPipelineID;
