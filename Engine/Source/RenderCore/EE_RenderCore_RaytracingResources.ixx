module;
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

export module EE_RenderCore_RaytracingResources;

export import EE_Core_WindowManager;
export import EE_RenderCore_VulkanTypes;
export import EE_RenderCore_VulkanUtilities;
export import EE_RenderCore_VulkanDescriptors;


export import <vector>;
export import <array>;
export import <string>;


export 
struct RaytracingResources
{
	std::vector<VkAccelerationStructureGeometryKHR> RaytracingAccelerationStructureGeometries;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> RaytracingAccelerationStructureBuildRangeInfos;

	std::vector<AllocatedBuffer> BlasBuffer;
	std::vector<VkAccelerationStructureKHR> BottomLevelAccelerationStructure;

	VkPhysicalDeviceProperties PdProperties {};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR RtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceAccelerationStructurePropertiesKHR  AsProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
	VkPhysicalDeviceDescriptorIndexingPropertiesEXT DiProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES_EXT };
	VkPhysicalDeviceExternalMemoryHostPropertiesEXT EmProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT };

	AllocatedBuffer RtSBTBuffer;
	VkStridedDeviceAddressRegionKHR RaygenRegion{};
	VkStridedDeviceAddressRegionKHR MissRegion{};
	VkStridedDeviceAddressRegionKHR HitRegion{};
	VkStridedDeviceAddressRegionKHR CallRegion{};

	AllocatedImage RaytracedImage;
	VkExtent2D RaytracedImageExtend;

	std::vector<VkDescriptorImageInfo> AlbedoImageInfos;
	std::vector<VkDescriptorImageInfo> AmbientOcclusionImageInfos;
	std::vector<VkDescriptorImageInfo> DepthImageInfos;
	std::vector<VkDescriptorImageInfo> MetallicImageInfos;
	std::vector<VkDescriptorImageInfo> NormalImageInfos;
	std::vector<VkDescriptorImageInfo> RoughnessImageInfos;

	std::vector<VkDescriptorBufferInfo> ModelVertexBufferInfos;
	std::vector<VkDescriptorBufferInfo> ModelIndexBufferInfos;
};