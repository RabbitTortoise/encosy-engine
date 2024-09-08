module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module EE_ECS_Systems_RaytracedRenderSystem;

import EE_Encosy_SystemThreaded;
import EE_ECS_Components_TransformComponent;
import EE_ECS_Components_MaterialComponent;
import EE_ECS_Components_CameraComponent;
import EE_ECS_Components_ModelMatrixComponent;
import EE_RenderCore;
import EE_Core_MatrixCalculations;
import EE_ECS_SystemData_CameraControllerSystem;
import EE_RenderCore_MeshLoader;
import EE_RenderCore_TextureLoader;
import EE_RenderCore_RenderPipelineManager;
import EE_RenderCore_Resources;
import EE_RenderCore_RaytracingResources;
import EE_RenderCore_AllocationHandler;

import <span>;
import <vector>;




export class EE_RaytracedRenderSystem : public SystemThreaded
{
	friend class SystemManager;

	SystemThreadedOptions ThreadedRunOptions =
	{
	.PreferRunAlone = true,
	.ThreadedUpdateCalls = false,
	.AllowPotentiallyUnsafeEdits = false,
	.AllowDestructiveEditsInThreads = false,
	.IgnoreThreadSaveFunctions = false,
	};

public:
	EE_RaytracedRenderSystem(RenderCore* engineRenderCore)
	{ 
		EngineRenderCore = engineRenderCore;
		MainMeshLoader = EngineRenderCore->GetMeshLoader();
		MainTextureLoader = EngineRenderCore->GetTextureLoader();
		MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();
		CoreResources = EngineRenderCore->GetRenderCoreResources();
		RtResources = EngineRenderCore->GetRaytracingResources();
		MainAllocationHandler = EngineRenderCore->GetAllocationHandler();
	}
	~EE_RaytracedRenderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		SetThreadedRunOptions(ThreadedRunOptions);

		auto cameraEntityInfo = GetEntityTypeInfo("CameraEntity");

		AddSystemDataForReading(&CameraSystemDataStorage);
		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForReading(&MaterialComponents);
		AddEntitiesForReading(cameraEntityInfo.Type, &CameraComponents);
		AddEntitiesForReading(cameraEntityInfo.Type, &CameraTransformComponents);
		
		CameraEntityType = cameraEntityInfo.Type;

		// Allocate a query pool for storing the needed size for every TLAS compaction.
		// Query compacted size
		VkQueryPoolCreateInfo qpci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
		qpci.queryCount = 1;
		qpci.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
		vkCreateQueryPool(CoreResources->vkDevice, &qpci, nullptr, &QueryPool);


		int threadCount = this->GetThreadCount();
		ThreadBuiltInstanceStructures.reserve(threadCount);
		ThreadBuiltInstancesData.reserve(threadCount);
		ThreadVariablesVector = std::vector<ThreadVariables>(threadCount, ThreadVariables());
	};

	void ClearTemporaryInstanceData()
	{

	}

	// Thread data initialization
	void PreUpdate(const int thread, const double deltaTime) override
	{
		size_t size = BuiltInstanceStructures.size();
		if (BuiltInstanceStructures.capacity() - size > 1000)
		{
			BuiltInstanceStructures.shrink_to_fit();
			BuiltInstancesData.shrink_to_fit();
		}
		BuiltInstanceStructures.clear();
		BuiltInstancesData.clear();

		ThreadBuiltInstanceStructures.clear();
		ThreadBuiltInstancesData.clear();

		int threadCount = this->GetThreadCount();
		for (size_t i = 0; i < threadCount; i++)
		{
			ThreadBuiltInstanceStructures.emplace_back(std::vector<VkAccelerationStructureInstanceKHR>());
			ThreadBuiltInstancesData.emplace_back(std::vector<RaytracingInstanceData>());
		}

		ThreadVariablesVector = std::vector<ThreadVariables>(threadCount, ThreadVariables());
	}; 

	void Update(const int thread, const double deltaTime) override
	{
		EE_CameraControllerSystemData csData = GetSystemData(&CameraSystemDataStorage);
		CurrentCameraComponent = GetEntityComponent(csData.MainCamera, CameraEntityType, &CameraComponents);
		CurrentCameraTransformComponent = GetEntityComponent(csData.MainCamera, CameraEntityType, &CameraTransformComponents);

		size_t entityCount = 0;
		for (const auto& span : TransformComponents.Storage)
		{
			entityCount += span.size();
		}
		BuiltInstanceStructures.reserve(entityCount);
		BuiltInstancesData.reserve(entityCount);

		int threadCount = this->GetThreadCount();
		for (int thread = 0; thread < threadCount; thread++)
		{
			ThreadBuiltInstanceStructures[thread].resize(this->GetThreadEntitiesCount(thread), VkAccelerationStructureInstanceKHR());
			ThreadBuiltInstancesData[thread].resize(this->GetThreadEntitiesCount(thread), RaytracingInstanceData());

			for (int i = 0; i < thread; i++)
			{
				ThreadVariablesVector[thread].InstaceIndexOffset += this->GetThreadEntitiesCount(i);
			}
		}
	};

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		EE_TransformComponent transformComponent = GetCurrentEntityComponent(thread, &TransformComponents);
		//EE_ModelMatrixComponent matrixComponent = GetCurrentEntityComponent(thread, &ModelMatrixComponents);
		EE_MaterialComponentRaytracing raytracingComponent = GetCurrentEntityComponent(thread, &MaterialComponents);

		uint32_t instanceIndex = static_cast<uint32_t>(ThreadVariablesVector[thread].InstaceIndexOffset + ThreadVariablesVector[thread].CurrentInstance);

		// Acceleration structure
		VkTransformMatrixKHR transform;

		glm::mat4 temp = glm::transpose(MatrixCalculations::CalculateModelMatrix(transformComponent));
		memcpy(&transform, &temp, sizeof(VkTransformMatrixKHR));

		VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
		addressInfo.accelerationStructure = RtResources->BottomLevelAccelerationStructure[raytracingComponent.RenderMesh];
		VkDeviceAddress blasAddress = CoreResources->DispatchTable.fp_vkGetAccelerationStructureDeviceAddressKHR(CoreResources->vkDevice, &addressInfo);

		VkAccelerationStructureInstanceKHR asInstance;
		asInstance.transform = transform;												// Transformation of the instance
		asInstance.instanceCustomIndex = instanceIndex;									// Identifies from what index the instance specific data is found.
		asInstance.mask = 0xFF;															//  Only be hit if rayMask & instance.mask != 0
		asInstance.instanceShaderBindingTableRecordOffset = 0;							// Same hit group is used for all objects
		asInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_KHR; //
		asInstance.accelerationStructureReference = blasAddress;							// The bottom level acceleration structure for the instance.

		ThreadBuiltInstanceStructures[thread][ThreadVariablesVector[thread].CurrentInstance] = asInstance;

		if (ThreadBuiltInstanceStructures[thread][ThreadVariablesVector[thread].CurrentInstance].instanceCustomIndex > 15624)
		{
			bool test = true;
		}			

		//Instance Data
		RaytracingInstanceData instanceData;
		instanceData.modelIndex = static_cast<uint32_t>(raytracingComponent.RenderMesh);
		instanceData.textureSet = static_cast<uint32_t>(raytracingComponent.TextureSet);
		instanceData.padding1 = 0;
		instanceData.padding2 = 0;
		instanceData.color = raytracingComponent.Color;
		instanceData.textureRepeat = raytracingComponent.TextureRepeat;

		ThreadBuiltInstancesData[thread][ThreadVariablesVector[thread].CurrentInstance] = instanceData;


		ThreadVariablesVector[thread].CurrentInstance += 1;
	}

	void PostUpdate(const int thread, const double deltaTime) override
	{
		// Create
		for (size_t i = 0; i < this->GetThreadCount(); i++)
		{
			BuiltInstanceStructures.insert(BuiltInstanceStructures.end(), 
				std::make_move_iterator(ThreadBuiltInstanceStructures[i].begin()),
				std::make_move_iterator(ThreadBuiltInstanceStructures[i].end())
				);
			BuiltInstancesData.insert(BuiltInstancesData.end(),
				std::make_move_iterator(ThreadBuiltInstancesData[i].begin()),
				std::make_move_iterator(ThreadBuiltInstancesData[i].end())
			);
			ThreadBuiltInstanceStructures[i].clear();
			ThreadBuiltInstancesData[i].clear();
		}

		if(BuiltInstanceStructures.size() > 0)
		{
			BuildTlas();
			Render();
		}
	};
	void Destroy() override 
	{
		vkDestroyQueryPool(CoreResources->vkDevice, QueryPool, nullptr);
	};

	void BuildTlas()
	{
		AllocatedBuffer tlasInstanceBuffer = MainAllocationHandler->CreateAndPopulateGPUBuffer(
			sizeof(VkAccelerationStructureInstanceKHR) * BuiltInstanceStructures.size(),
			BuiltInstanceStructures.data(),
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			false);
		
		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(tlasInstanceBuffer);
		});

		VkBufferDeviceAddressInfo tlasInstanceAddressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = tlasInstanceBuffer.buffer};
		VkDeviceAddress tlasInstanceAddress = vkGetBufferDeviceAddress(CoreResources->vkDevice, &tlasInstanceAddressInfo);

		VkAccelerationStructureGeometryKHR tlasGeometry = {};
		tlasGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		tlasGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		tlasGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		tlasGeometry.geometry.instances.arrayOfPointers = false;
		tlasGeometry.geometry.instances.data.deviceAddress = tlasInstanceAddress;

		// Query tlas size
		VkAccelerationStructureBuildGeometryInfoKHR tlasInfo{ };
		tlasInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		tlasInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		tlasInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		tlasInfo.geometryCount = 1;
		tlasInfo.pGeometries = &tlasGeometry;
		
		tlasInfo.flags =
			VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
			VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR |
			VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;


		VkAccelerationStructureBuildSizesInfoKHR tlasSizeInfo{  };
		tlasSizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		tlasSizeInfo.pNext = nullptr;

		uint32_t maxInstanceCount = BuiltInstanceStructures.size();
		CoreResources->DispatchTable.fp_vkGetAccelerationStructureBuildSizesKHR(
			CoreResources->vkDevice,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&tlasInfo,
			&maxInstanceCount,
			&tlasSizeInfo);

		AllocatedBuffer tempTlasBuffer = MainAllocationHandler->CreateGPUBuffer(
			tlasSizeInfo.accelerationStructureSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			false);

		VkDeviceSize minAlignment = static_cast<VkDeviceSize>(RtResources->AsProperties.minAccelerationStructureScratchOffsetAlignment);

		AllocatedBuffer tlasScratchBuffer = MainAllocationHandler->CreateGPUBufferWithAlignment(
			tlasSizeInfo.buildScratchSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
			minAlignment,
			false);

		VkBufferDeviceAddressInfo tlasScratchAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = tlasScratchBuffer.buffer };
		auto tlasScratchAddress = vkGetBufferDeviceAddress(CoreResources->vkDevice, &tlasScratchAdressInfo);

		VkAccelerationStructureCreateInfoKHR tlasCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		tlasCreateInfo.buffer = tempTlasBuffer.buffer;
		tlasCreateInfo.offset = 0;
		tlasCreateInfo.size = tlasSizeInfo.accelerationStructureSize;
		tlasCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

		VkAccelerationStructureKHR tempTLAS = VkAccelerationStructureKHR();

		MainAllocationHandler->CreateAccelerationStructure(tlasCreateInfo, tempTLAS);

		// Build top level acceleration structure
		tlasInfo.dstAccelerationStructure = tempTLAS;
		tlasInfo.scratchData.deviceAddress = tlasScratchAddress;
		VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo{ };
		tlasRangeInfo.primitiveCount = BuiltInstanceStructures.size();
		VkAccelerationStructureBuildRangeInfoKHR* tlas_ranges[] = { &tlasRangeInfo };
		MainAllocationHandler->BuildAccelerationStructure(tlasInfo, tlas_ranges, QueryPool);

		// Get the compacted size result back
		VkDeviceSize compactSize = 0;
		vkGetQueryPoolResults(CoreResources->vkDevice, QueryPool, 0, 1, sizeof(VkDeviceSize),
			&compactSize, sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

		AllocatedBuffer tlasBuffer = MainAllocationHandler->CreateGPUBuffer(
			compactSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
			false);

		VkAccelerationStructureKHR& currentFrameTLAS = CoreResources->GetCurrentFrame().TopLevelAccelerationStructure;
		currentFrameTLAS = CreateCompactedTlas(compactSize, tlasBuffer.buffer, tempTLAS, QueryPool);


		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			CoreResources->DispatchTable.fp_vkDestroyAccelerationStructureKHR(CoreResources->vkDevice, currentFrameTLAS, nullptr);
			});
		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(tlasBuffer);
			});
		MainAllocationHandler->DestroyBuffer(tempTlasBuffer);
		MainAllocationHandler->DestroyBuffer(tlasScratchBuffer);
	}

	VkAccelerationStructureKHR CreateCompactedTlas(
		VkDeviceSize compactSize,
		VkBuffer tlasBuffer,
		VkAccelerationStructureKHR oldAccelerationStucture,
		VkQueryPool queryPool
	)
	{
		//Creating a compact version of the AS
		VkAccelerationStructureCreateInfoKHR asCreateInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
		asCreateInfo.size = compactSize;
		asCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		asCreateInfo.buffer = tlasBuffer;
		asCreateInfo.offset = 0;

		// Create new structure and copy the original TLAS to a compact version
		VkAccelerationStructureKHR compactedAccelerationStructure = nullptr;
		MainAllocationHandler->CreateAccelerationStructure(asCreateInfo, compactedAccelerationStructure, false);
		MainAllocationHandler->CopyAccelerationStructure(oldAccelerationStucture, compactedAccelerationStructure, true);
		return compactedAccelerationStructure;
	}




	void Render()
	{
		RaytracingPipeline UsedPipeline = MainRenderPipelineManager->GetEngineRtPipeline(EngineRTPipelines::PBR);


		// Allocate buffer data						//
		// And setup data needed in descriptor sets //

		// Set 0 - Result Image//
		// Bind 0 - Result Image
		AllocatedImage resultImage = RtResources->RaytracedImage;

		// Set 1 - Top Level Acceleration Structure + Camera Data //
		// Bind 0 - TLAS
		VkAccelerationStructureKHR& currentFrameTLAS = CoreResources->GetCurrentFrame().TopLevelAccelerationStructure;
		// Bind 1 - Camera Data
		auto projection = CurrentCameraComponent.Projection;
		// Invert the Y direction on projection matrix so +Y is up instead of down
		projection[1][1] *= -1;
		RaytracingCameraProperties raytracingCameraData
		{
			.viewInverse = glm::inverse(CurrentCameraComponent.View),
			.projInverse = glm::inverse(projection),
			.cameraPosition = glm::vec4(CurrentCameraTransformComponent.Position, 1.0f)
		};
		AllocatedBuffer cameraDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(RaytracingCameraProperties), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		MainAllocationHandler->WriteToBuffer(cameraDataBuffer, &raytracingCameraData);
		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(cameraDataBuffer);
			});


		// Set 2 - Lighting Data //
		// Bind 0 - Lighting Buffer
		LitLightingData lighting = CoreResources->GlobalLightingData;
		LitLightingDataInShader  lightingData =
		{
			.ambientLightColor = glm::vec4(lighting.ambientLightColor, lighting.ambientLightStrength),
			.directionalLightColor = glm::vec4(lighting.directionalLightColor, lighting.directionalLightStrength),
			.directionalLightDir = glm::vec4(lighting.directionalLightDir, 0.0f)
		};
		//Invert light y direction as our view matrix y direction is also inverted.
		lightingData.directionalLightDir.y *= -1;

		AllocatedBuffer lightingDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(LitLightingDataInShader), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		MainAllocationHandler->WriteToBuffer(lightingDataBuffer, &lightingData);
		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(lightingDataBuffer);
			});

		// Set 3 - Instance Data //
		// Bind 0 - Instance Buffer
		AllocatedBuffer instanceDataBuffer = MainAllocationHandler->CreateAndPopulateGPUBuffer(sizeof(RaytracingInstanceData) * BuiltInstancesData.size(), BuiltInstancesData.data(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		CoreResources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(instanceDataBuffer);
			});

		// Set 4 - Model Data //
		// Bind 0 - Vertex Buffer
		std::vector<VkDescriptorBufferInfo>& vertexBufferInfos = RtResources->ModelVertexBufferInfos;
		// Bind 1 - Index Buffer
		std::vector<VkDescriptorBufferInfo>& indexBufferInfos = RtResources->ModelIndexBufferInfos;

		// Set 5 - Texture Data //
		// Bind 0 - Image Sampler
		VkSampler imageSampler = CoreResources->DefaultSamplerNearest;
		// Bind 1 - Albedo Textures
		std::vector<VkDescriptorImageInfo>& albedoTextureInfos = RtResources->AlbedoImageInfos;
		// Bind 2 - AO Textures
		std::vector<VkDescriptorImageInfo>& aoTextureInfos = RtResources->AmbientOcclusionImageInfos;
		// Bind 3 - Depth Textures
		std::vector<VkDescriptorImageInfo>& depthTextureInfos = RtResources->DepthImageInfos;
		// Bind 4 - Metallic Textures
		std::vector<VkDescriptorImageInfo>& metallicTextureInfos = RtResources->MetallicImageInfos;
		// Bind 5 - Normal Textures
		std::vector<VkDescriptorImageInfo>& normalTextureInfos = RtResources->NormalImageInfos;
		// Bind 6 - Roughness Textures
		std::vector<VkDescriptorImageInfo>& roughnessTextureInfos = RtResources->RoughnessImageInfos;

		// Allocate descriptor sets //

		// Set 0 - Result Image //
		VkDescriptorSet rayGenDescriptorSet= CoreResources->GetCurrentFrame().vkFrameDescriptors
			.Allocate(CoreResources->vkDevice, UsedPipeline.RayGenDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteImage(0, resultImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
			writer.UpdateSet(CoreResources->vkDevice, rayGenDescriptorSet);
		}

		// Set 1 - Top Level Acceleration Structure + Camera Data c//
		// Create descriptor set extension for acceleration structure
		VkWriteDescriptorSetAccelerationStructureKHR tlasDescriptorExtension{};
		tlasDescriptorExtension.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		tlasDescriptorExtension.accelerationStructureCount = 1;
		tlasDescriptorExtension.pAccelerationStructures = &currentFrameTLAS;

		VkDescriptorSet tlasDescriptorSet = CoreResources->GetCurrentFrame().vkFrameDescriptors
			.Allocate(CoreResources->vkDevice, UsedPipeline.TLASDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteAccelerationStructure(0, tlasDescriptorExtension);
			writer.WriteBuffer(1, cameraDataBuffer.buffer, sizeof(RaytracingCameraProperties), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.UpdateSet(CoreResources->vkDevice, tlasDescriptorSet);
		}

		// Set 2 - Lighting Data //
		VkDescriptorSet lightingDataDescriptorSet = CoreResources->GetCurrentFrame().vkFrameDescriptors.Allocate(CoreResources->vkDevice, UsedPipeline.LightingDataDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, lightingDataBuffer.buffer, sizeof(LitLightingDataInShader), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.UpdateSet(CoreResources->vkDevice, lightingDataDescriptorSet);
		}

		// Set 3 - Instance Data //
		VkDescriptorSet instanceDataDescriptorSet = CoreResources->GetCurrentFrame().vkFrameDescriptors.Allocate(CoreResources->vkDevice, UsedPipeline.InstanceDataDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, instanceDataBuffer.buffer, instanceDataBuffer.info.size, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
			writer.UpdateSet(CoreResources->vkDevice, instanceDataDescriptorSet);
		}

		// Set 4 - Model Data //
			VkDescriptorSet modelDataDescriptorSet = CoreResources->GetCurrentFrame().vkFrameDescriptors.AllocateDynamic(CoreResources->vkDevice, UsedPipeline.ModelDataDescriptorSetLayout, RtResources->ModelVertexBufferInfos.size());
		{
			DescriptorWriter writer;
			writer.WriteBufferArray(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBufferInfos);
			writer.WriteBufferArray(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, indexBufferInfos);
			writer.UpdateSet(CoreResources->vkDevice, modelDataDescriptorSet);
			}

		// Set 5 - Texture Data //
		VkDescriptorSet textureDataDescriptorSet = CoreResources->GetCurrentFrame().vkFrameDescriptors.AllocateDynamic(CoreResources->vkDevice, UsedPipeline.TextureDataDescriptorSetLayout, RtResources->AlbedoImageInfos.size());
		{
			DescriptorWriter writer;
			writer.WriteSampler(0, imageSampler, VK_DESCRIPTOR_TYPE_SAMPLER);
			writer.WriteImageArray(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, albedoTextureInfos);
			writer.WriteImageArray(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, aoTextureInfos);
			writer.WriteImageArray(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, depthTextureInfos);
			writer.WriteImageArray(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, metallicTextureInfos);
			writer.WriteImageArray(5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, normalTextureInfos);
			writer.WriteImageArray(6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, roughnessTextureInfos);
			writer.UpdateSet(CoreResources->vkDevice, textureDataDescriptorSet);
		}

		vkCmdBindPipeline(CoreResources->GetCurrentFrame().vkMainCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, UsedPipeline.Pipeline);

		std::vector<VkDescriptorSet> FrameDescriptorSets;
		FrameDescriptorSets.push_back(rayGenDescriptorSet);
		FrameDescriptorSets.push_back(tlasDescriptorSet);
		FrameDescriptorSets.push_back(lightingDataDescriptorSet);
		FrameDescriptorSets.push_back(instanceDataDescriptorSet);
		FrameDescriptorSets.push_back(modelDataDescriptorSet);
		FrameDescriptorSets.push_back(textureDataDescriptorSet);

		vkCmdBindDescriptorSets(CoreResources->GetCurrentFrame().vkMainCommandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, UsedPipeline.Layout, 0, FrameDescriptorSets.size(), FrameDescriptorSets.data(), 0, nullptr);

		auto size = RtResources->RaytracedImageExtend;
		size.width *= CoreResources->RenderScale;
		size.height *= CoreResources->RenderScale;
		CoreResources->DispatchTable.fp_vkCmdTraceRaysKHR(CoreResources->GetCurrentFrame().vkMainCommandBuffer, &RtResources->RaygenRegion, &RtResources->MissRegion, &RtResources->HitRegion, &RtResources->CallRegion, size.width, size.height, 1);
	}

private:

	struct alignas(64) ThreadVariables
	{
		size_t InstaceIndexOffset = 0;
		size_t CurrentInstance = 0;
	};

	RenderCore* EngineRenderCore;
	RenderCoreResources* CoreResources;
	RaytracingResources* RtResources;
	MeshLoader* MainMeshLoader;
	TextureLoader* MainTextureLoader;
	RenderPipelineManager* MainRenderPipelineManager;
	AllocationHandler* MainAllocationHandler;

	EntityType CameraEntityType;
	EE_CameraComponent CurrentCameraComponent;
	EE_TransformComponent CurrentCameraTransformComponent;

	// Component Storages
	ReadOnlyComponentStorage<EE_TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<EE_MaterialComponentRaytracing> MaterialComponents;
	ReadOnlyAlwaysFetchedStorage<EE_CameraComponent> CameraComponents;
	ReadOnlyAlwaysFetchedStorage<EE_TransformComponent> CameraTransformComponents;
	ReadOnlySystemDataStorage<EE_CameraControllerSystemData> CameraSystemDataStorage;
	
	// Instance calculation
	VkQueryPool QueryPool{ VK_NULL_HANDLE };
	 
	std::vector<VkAccelerationStructureInstanceKHR> BuiltInstanceStructures;
	std::vector<RaytracingInstanceData> BuiltInstancesData;
	size_t PreviousInstanceGroupIndex = 0;

	// Thread Data
	std::vector<std::vector<VkAccelerationStructureInstanceKHR>> ThreadBuiltInstanceStructures;
	std::vector<std::vector<RaytracingInstanceData>> ThreadBuiltInstancesData;
	std::vector<ThreadVariables> ThreadVariablesVector;

};
