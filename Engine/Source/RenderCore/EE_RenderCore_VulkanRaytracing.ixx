
module;
#include <vulkan/vulkan.h>
#include <fmt/format.h>

#include "fmt/ranges.h"

export module EE_RenderCore_VulkanRaytracing;

import EE_RenderCore_VulkanErrorLogger;
import EE_RenderCore_MeshLoader;
import EE_RenderCore_TextureLoader;
import EE_RenderCore_Resources;
import EE_RenderCore_RaytracingResources;
import EE_RenderCore_AllocationHandler;
import EE_RenderCore_RenderPipelineManager;
import <vector>;
import <deque>;





export
class VulkanRaytracing
{
public:
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};


	VulkanRaytracing(RenderCoreResources* resources, RaytracingResources* rtResources, AllocationHandler* allocationHandler, MeshLoader* meshLoader, TextureLoader* mainTextureLoader, RenderPipelineManager* pipelineManager)
	{
		RtResources = rtResources;
		MainMeshLoader = meshLoader;
		MainTextureLoader = mainTextureLoader;
		CoreResources = resources;
		MainAllocationHandler = allocationHandler;
		MainRenderPipelineManager = pipelineManager;
	}
	~VulkanRaytracing(){};


	void InitRayTracing()
	{
		// Requesting ray tracing properties
		VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };

		prop2.pNext = &RtResources->RtProperties;
		RtResources->RtProperties.pNext = &RtResources->AsProperties;
		RtResources->AsProperties.pNext = &RtResources->DiProperties;
		RtResources->DiProperties.pNext = &RtResources->EmProperties;
		vkGetPhysicalDeviceProperties2(CoreResources->vkChosenGPU, &prop2);
		RtResources->PdProperties = prop2.properties;
		

		// Check that ray recursion depth is large enough.
		if (RtResources->RtProperties.maxRayRecursionDepth <= 1) 
		{
			throw std::runtime_error("Device fails to support ray recursion (m_rtProperties.maxRayRecursionDepth <= 1)");
		}

		for (MeshID meshID = 0; meshID <= static_cast<MeshID>(EngineMesh::Torus); ++meshID)
		{
			RegisterMeshForRaytracingUsage(meshID);
		}
	}

	TextureSetID RegisterTextureSetForRaytracingUsage(PBRTextureSet set)
	{
		RegisteredTextureSets.push_back(set);
		return(RegisteredTextureSets.size() - 1);
	}

	void RegisterMeshForRaytracingUsage(MeshID mesh)
	{
		RegisteredMeshes.push_back(mesh);
	}

	void CreateTextureSets()
	{
		RtResources->AlbedoImageInfos.clear();
		RtResources->AmbientOcclusionImageInfos.clear();
		RtResources->DepthImageInfos.clear();
		RtResources->MetallicImageInfos.clear();
		RtResources->NormalImageInfos.clear();
		RtResources->RoughnessImageInfos.clear();

		if(RegisteredTextureSets.size() == 0)
		{
			fmt::println("WARNING: No textures registered for raytracing usage!");
		}

		CreateErrorTextureSet();

		for (const auto set : RegisteredTextureSets)
		{
			auto albedo = MainTextureLoader->GetTexture(			set.Albedo);
			auto ambientOcclusion = MainTextureLoader->GetTexture(	set.AmbientOcclusion);
			auto depth = MainTextureLoader->GetTexture(				set.Depth);
			auto metallic = MainTextureLoader->GetTexture(			set.Metallic);
			auto normal = MainTextureLoader->GetTexture(			set.Normal);
			auto roughness = MainTextureLoader->GetTexture(			set.Roughness);

			VkDescriptorImageInfo albedoInfo = VkDescriptorImageInfo{
				.sampler = nullptr,
				.imageView = albedo.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			auto ambientOcclusionInfo = albedoInfo;
			auto depthInfo = albedoInfo;
			auto metallicInfo = albedoInfo;
			auto normalInfo = albedoInfo;
			auto roughnessInfo = albedoInfo;

			ambientOcclusionInfo.imageView =	ambientOcclusion.imageView;
			depthInfo.imageView =				depth.imageView;
			metallicInfo.imageView =			metallic.imageView;
			normalInfo.imageView =				normal.imageView;
			roughnessInfo.imageView =			roughness.imageView;

			RtResources->AlbedoImageInfos.emplace_back(albedoInfo);
			RtResources->AmbientOcclusionImageInfos.emplace_back(ambientOcclusionInfo);
			RtResources->DepthImageInfos.emplace_back(depthInfo);
			RtResources->MetallicImageInfos.emplace_back(metallicInfo);
			RtResources->NormalImageInfos.emplace_back(normalInfo);
			RtResources->RoughnessImageInfos.emplace_back(roughnessInfo);
		}
	}

	void CreateErrorTextureSet()
	{
		auto albedo = MainTextureLoader->GetEngineTexture(EngineTextures::ErrorCheckerBoard);
		auto ambientOcclusion = MainTextureLoader->GetEngineTexture(EngineTextures::White);
		auto depth = MainTextureLoader->GetEngineTexture(EngineTextures::Black);
		auto metallic = MainTextureLoader->GetEngineTexture(EngineTextures::White);
		auto normal = MainTextureLoader->GetEngineTexture(EngineTextures::NeutralNormal);
		auto roughness = MainTextureLoader->GetEngineTexture(EngineTextures::White);

		VkDescriptorImageInfo albedoInfo = VkDescriptorImageInfo{
				.sampler = nullptr,
				.imageView = albedo.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		auto ambientOcclusionInfo = albedoInfo;
		auto depthInfo = albedoInfo;
		auto metallicInfo = albedoInfo;
		auto normalInfo = albedoInfo;
		auto roughnessInfo = albedoInfo;

		ambientOcclusionInfo.imageView = ambientOcclusion.imageView;
		depthInfo.imageView = depth.imageView;
		metallicInfo.imageView = metallic.imageView;
		normalInfo.imageView = normal.imageView;
		roughnessInfo.imageView = roughness.imageView;

		RtResources->AlbedoImageInfos.emplace_back(albedoInfo);
		RtResources->AmbientOcclusionImageInfos.emplace_back(ambientOcclusionInfo);
		RtResources->DepthImageInfos.emplace_back(depthInfo);
		RtResources->MetallicImageInfos.emplace_back(metallicInfo);
		RtResources->NormalImageInfos.emplace_back(normalInfo);
		RtResources->RoughnessImageInfos.emplace_back(roughnessInfo);
	}

	void CreateModelBufferInfos()
	{
		RtResources->ModelVertexBufferInfos.clear();
		RtResources->ModelIndexBufferInfos.clear();

		std::vector<VkBuffer> ModelVertexBuffers;
		std::vector<VkBuffer> ModelIndexBuffers;

		ModelVertexBuffers.reserve(RegisteredMeshes.size());
		ModelIndexBuffers.reserve(RegisteredMeshes.size());

		for (auto meshId : RegisteredMeshes)
		{
			ModelVertexBuffers.emplace_back(MainMeshLoader->GetMeshBuffers(meshId)->vertexBuffer.buffer);
			ModelIndexBuffers.emplace_back(MainMeshLoader->GetMeshBuffers(meshId)->indexBuffer.buffer);

		}

		for (const auto& buffer : ModelVertexBuffers)
		{
			VkDescriptorBufferInfo info = VkDescriptorBufferInfo{
				
				.buffer = buffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE

			};
			RtResources->ModelVertexBufferInfos.emplace_back(info);
		}
		for (const auto& buffer : ModelIndexBuffers)
		{
			VkDescriptorBufferInfo info = VkDescriptorBufferInfo{
				.buffer = buffer,
				.offset = 0,
				.range = VK_WHOLE_SIZE
			};
			RtResources->ModelIndexBufferInfos.emplace_back(info);
		}
	}

	void RecalculateBottomLevelAccelerationStructures()
	{
		RtResources->RaytracingAccelerationStructureGeometries.clear();
		RtResources->RaytracingAccelerationStructureBuildRangeInfos.clear();

		int index = -1;
		for (auto meshId : RegisteredMeshes)
		{
			index++;
			auto meshData = MainMeshLoader->GetMeshAllocatedData(meshId);
			CalculateBottomLevelAccelerationStructure(index, meshData);
		}
		fmt::println("Bottom level acceleration structures created for raytracing");
	}

	void CalculateBottomLevelAccelerationStructure(size_t index, MeshAllocatedData& meshData)
	{
		//
		// Creation of bottomLevelAccelerationStructures
		//
		// Assign correct geometry information for the acceleration structure
		VkAccelerationStructureBuildGeometryInfoKHR asInfo{ };
		asInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		asInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		asInfo.geometryCount = 1;
		asInfo.pGeometries = &meshData.RaytracingBufferInfo->accelerationStructureGeometry;

		asInfo.flags =
			VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
			VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

		std::vector<uint32_t> maxPrimitivesCount;
		maxPrimitivesCount.emplace_back(meshData.RaytracingBufferInfo->accelerationStructureBuildRangeInfo.primitiveCount);

		// Query the size of the acceleration structure
		VkAccelerationStructureBuildSizesInfoKHR asSizeInfo{  };
		asSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		asSizeInfo.pNext = nullptr;

		CoreResources->DispatchTable.fp_vkGetAccelerationStructureBuildSizesKHR(
			CoreResources->vkDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&asInfo,
			maxPrimitivesCount.data(),
			&asSizeInfo);

		// Create buffers for bottom level acceleration structure creation
		
		AllocatedBuffer blasScratchBuffer = MainAllocationHandler->CreateGPUBuffer(asSizeInfo.buildScratchSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true);

		AllocatedBuffer tempBlasBuffer = MainAllocationHandler->CreateGPUBuffer(asSizeInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, false);

		VkAccelerationStructureKHR tempAccelerationStructure = VkAccelerationStructureKHR();

		// Create blas
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.buffer = tempBlasBuffer.buffer;
		asCreateInfo.offset = 0;
		asCreateInfo.size = asSizeInfo.accelerationStructureSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		MainAllocationHandler->CreateAccelerationStructure(asCreateInfo, tempAccelerationStructure, false);

		asInfo.dstAccelerationStructure = tempAccelerationStructure;

		VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = blasScratchBuffer.buffer };
		auto scratchAddress = vkGetBufferDeviceAddress(CoreResources->vkDevice, &deviceAdressInfo);
		asInfo.scratchData.deviceAddress = scratchAddress;

		VkAccelerationStructureBuildRangeInfoKHR blas_range = {
			meshData.RaytracingBufferInfo->accelerationStructureBuildRangeInfo
		};
		VkAccelerationStructureBuildRangeInfoKHR* blas_ranges[] = {
			&blas_range
		};

		// Allocate a query pool for storing the needed size for every BLAS compaction.
		VkQueryPool queryPool{ VK_NULL_HANDLE };
		// Query compacted size
		VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		qpci.queryCount = 1;
		qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		vkCreateQueryPool(CoreResources->vkDevice, &qpci, nullptr, &queryPool);

		MainAllocationHandler->BuildAccelerationStructure(asInfo, blas_ranges, queryPool);

		// Get the compacted size result back
		VkDeviceSize compactSize = 0;
		vkGetQueryPoolResults(CoreResources->vkDevice, queryPool, 0, 1, sizeof(VkDeviceSize),
			&compactSize, sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		RtResources->BlasBuffer.emplace_back(MainAllocationHandler->CreateGPUBuffer(compactSize, 
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, true));

		VkAccelerationStructureKHR compactedBlAs = CreateCompactedBlas(
			asInfo, blas_range, compactSize, RtResources->BlasBuffer[index].buffer, tempAccelerationStructure, queryPool);

		RtResources->BottomLevelAccelerationStructure.emplace_back(compactedBlAs);
		RtResources->RaytracingAccelerationStructureGeometries.emplace_back(meshData.RaytracingBufferInfo->accelerationStructureGeometry);
		RtResources->RaytracingAccelerationStructureBuildRangeInfos.emplace_back(meshData.RaytracingBufferInfo->accelerationStructureBuildRangeInfo);

		vkDestroyQueryPool(CoreResources->vkDevice, queryPool, nullptr);
		MainAllocationHandler->DestroyBuffer(tempBlasBuffer);
	}


	VkAccelerationStructureKHR CreateCompactedBlas(
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo,
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo,
		VkDeviceSize compactSize,
		VkBuffer blasBuffer,
		VkAccelerationStructureKHR oldAccelerationStucture,
		VkQueryPool queryPool
	) 
	{
		// Creating a compact version of the AS
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.size = compactSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		asCreateInfo.buffer = blasBuffer;
		asCreateInfo.offset = 0;

		// Create new structure and copy the original BLAS to a compact version
		VkAccelerationStructureKHR compactedAccelerationStructure = nullptr;
		MainAllocationHandler->CreateAccelerationStructure(asCreateInfo, compactedAccelerationStructure, true);
		MainAllocationHandler->CopyAccelerationStructure(oldAccelerationStucture, compactedAccelerationStructure, true);
		return compactedAccelerationStructure;
	}

	void CreateRtShaderBindingTable()
	{
		const RaytracingPipeline& UsedPipeline = MainRenderPipelineManager->GetEngineRtPipeline(EngineRTPipelines::PBR);

		uint32_t missCount{ 2 };
		uint32_t hitCount{ 1 };
		auto     handleCount = 1 + missCount + hitCount;
		uint32_t handleSize = RtResources->RtProperties.shaderGroupHandleSize;

		// The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
		uint32_t handleSizeAligned = AlignUp(handleSize, RtResources->RtProperties.shaderGroupHandleAlignment);

		RtResources->RaygenRegion.stride = AlignUp(handleSizeAligned, RtResources->RtProperties.shaderGroupBaseAlignment);
		RtResources->RaygenRegion.size = RtResources->RaygenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
		RtResources->MissRegion.stride = handleSizeAligned;
		RtResources->MissRegion.size = AlignUp(missCount * handleSizeAligned, RtResources->RtProperties.shaderGroupBaseAlignment);
		RtResources->HitRegion.stride = handleSizeAligned;
		RtResources->HitRegion.size = AlignUp(hitCount * handleSizeAligned, RtResources->RtProperties.shaderGroupBaseAlignment);

		// Get the shader group handles
		uint32_t dataSize = handleCount * handleSize;
		std::vector<uint8_t> handles(dataSize);

		VK_CHECK(CoreResources->DispatchTable.fp_vkGetRayTracingShaderGroupHandlesKHR(CoreResources->vkDevice, UsedPipeline.Pipeline, 0, handleCount, dataSize, handles.data()));

		// Allocate a buffer for storing the SBT.
		VkDeviceSize sbtSize = RtResources->RaygenRegion.size + RtResources->MissRegion.size + RtResources->HitRegion.size + RtResources->CallRegion.size;
		RtResources->RtSBTBuffer = MainAllocationHandler->CreateGPUBuffer(sbtSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
			true);

		// Find the SBT addresses of each group
		VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, RtResources->RtSBTBuffer.buffer };
		VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(CoreResources->vkDevice, &info);
		RtResources->RaygenRegion.deviceAddress = sbtAddress;
		RtResources->MissRegion.deviceAddress = sbtAddress + RtResources->RaygenRegion.size;
		RtResources->HitRegion.deviceAddress = sbtAddress + RtResources->RaygenRegion.size + RtResources->MissRegion.size;

		MainAllocationHandler->CopyRtHandleData(RtResources->RtSBTBuffer, handles, handleSize, RtResources->RaygenRegion, RtResources->MissRegion, missCount, RtResources->HitRegion, hitCount);


	}

	void CreateRaytracingStorageImage()
	{
		VkExtent2D extent = VkExtent2D(CoreResources->vkWindowExtent.width, CoreResources->vkWindowExtent.height);
		RtResources->RaytracedImageExtend = extent;
		VkExtent3D size = VkExtent3D(extent.width, extent.height, 1);
		VkFormat format = CoreResources->vkDrawImage.imageFormat;
		VkImageUsageFlags usage = 
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		RtResources->RaytracedImage = MainAllocationHandler->AllocateImage(size, format, usage, false, false);
	}

	void ResizeRaytracingStorageImage()
	{
		DeleteRaytracingStorageImage();
		CreateRaytracingStorageImage();
	}

	void DeleteRaytracingStorageImage()
	{
		MainAllocationHandler->DestroyImage(RtResources->RaytracedImage);
	}

private:

	// Formula from: https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/#shaderbindingtable
	template <class T> requires std::integral<T>
	static constexpr T AlignUp(T x, size_t a) noexcept 
	{
		return T((x + (static_cast<T>(a) - 1)) & ~static_cast<T>(T(a - 1)));
	}

	std::vector<PBRTextureSet> RegisteredTextureSets;
	std::vector<MeshID> RegisteredMeshes;

	RenderPipelineManager* MainRenderPipelineManager;
	MeshLoader* MainMeshLoader;
	TextureLoader* MainTextureLoader;
	RaytracingResources* RtResources;
	RenderCoreResources* CoreResources;
	AllocationHandler* MainAllocationHandler;
};
