module;
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

export module RenderCore.VulkanTypes;

import <vector>;


export typedef size_t MeshID;
export typedef size_t TextureID;
export typedef size_t TextureSetID;
export typedef size_t ShaderID;
export typedef size_t RenderPipelineID;


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

// Holds all the position data for a vertex. Used when creating raytracing meshes.
export
struct VertexPos {
	glm::vec3 position;
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
	VkDeviceAddress indexBufferAddress;
	VkDeviceAddress vertexBufferAddress;
};

export
struct GPURaytracingMeshBuffer{
	AllocatedBuffer indexBuffer;
	AllocatedBuffer vertexBuffer;
	VkDeviceAddress indexBufferAddress;
	VkDeviceAddress vertexBufferAddress;
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry;
	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
};

// Holds the loaded mesh vertex and index data
export
struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

export struct MeshAllocatedData
{
	Mesh* MeshInfo;
	GPUMeshBuffers* BufferInfo;
	GPURaytracingMeshBuffer* RaytracingBufferInfo;
};


// Holds the resources needed for model matrices
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
struct RaytracingPushConstants
{
	VkDeviceAddress vertexBufferAddress;
	VkDeviceAddress indexBufferAddress;
	glm::vec4  clearColor;
};

export
struct RaytracingCameraProperties
{
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
	glm::vec4 cameraPosition;
};

export
struct RaytracingMaterialProperties
{
	VkDeviceAddress vertexBufferAddress;
	VkDeviceAddress indexBufferAddress;
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

export
struct PaletteInfo
{
	float h = 0;
	std::vector<glm::vec<3, float>> colors;
	std::vector<glm::vec<2, float>> sl;
	bool enabled;
};

export
struct PaletteOptions
{
	float h = 0;
	int paletteSize = 0;
	float enabled = 1;
	float padding2 = 0;
};

export
struct RaytracingInstanceData
{
	uint32_t modelIndex = 0;
	uint32_t textureSet = 0;
	uint32_t padding1 = 0;
	uint32_t padding2 = 0;
	glm::vec3 color = glm::vec3(1);
	float textureRepeat = 0;
};

export
class PBRTextureSet
{
public:
	PBRTextureSet(TextureID albedo, TextureID ambientOcclusion, TextureID depth, TextureID metallic, TextureID normal, TextureID roughness) :
		Albedo(albedo), AmbientOcclusion(ambientOcclusion), Depth(depth), Metallic(metallic), Normal(normal), Roughness(roughness) {}

	TextureID Albedo = 0;
	TextureID AmbientOcclusion = 0;
	TextureID Depth = 0;
	TextureID Metallic = 0;
	TextureID Normal = 0;
	TextureID Roughness = 0;
};
