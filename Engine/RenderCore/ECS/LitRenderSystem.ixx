module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>

export module Systems.LitRenderSystem;

import EncosyCore.SystemThreaded;
import Components.TransformComponent;
import Components.MaterialComponent;
import Components.CameraComponent;
import Components.ModelMatrixComponent;
import EncosyEngine.RenderCore;
import EncosyEngine.MatrixCalculations;
import SystemData.CameraControllerSystem;
import RenderCore.MeshLoader;
import RenderCore.TextureLoader;
import RenderCore.RenderPipelineManager;
import RenderCore.Resources;
import RenderCore.AllocationHandler;

import <span>;
import <vector>;
import <functional>;
import <algorithm>;
import <iterator>;


struct InstanceProperties
{
	MaterialComponentLit Material = {};
	std::vector<glm::mat4> ModelMatrices;
};
struct ThreadInstanceCalculationData
{
	size_t previousInstanceIndex = -1;
	std::array<size_t, 7> padding = {0,0,0,0,0,0,0};
};


export class LitRenderSystem : public SystemThreaded
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
	LitRenderSystem(RenderCore* engineRenderCore)
	{ 
		EngineRenderCore = engineRenderCore;
		MainMeshLoader = EngineRenderCore->GetMeshLoader();
		MainTextureLoader = EngineRenderCore->GetTextureLoader();
		MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();
		Resources = EngineRenderCore->GetRenderCoreResources();
		MainAllocationHandler = EngineRenderCore->GetAllocationHandler();
	}
	~LitRenderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		RunSyncPoint = SystemSyncPoint::WithEngineSystems;
		RunAfterSpecificSystem = "ModelMatrixBuilderSystem";
		SetThreadedRunOptions(ThreadedRunOptions);

		auto cameraEntityInfo = GetEntityTypeInfo("CameraEntity");

		AddSystemDataForReading(&CameraSystemDataStorage);
		AddComponentQueryForReading(&TransformComponents);
		AddComponentQueryForReading(&MaterialComponents);
		AddComponentQueryForReading(&ModelMatrixComponents);
		AddEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		
		CameraEntityType = cameraEntityInfo.Type;

		BuiltInstances = std::vector<InstanceProperties>();
		InstanceThreadVectors = std::vector<std::vector<InstanceProperties>>(GetThreadCount(), std::vector<InstanceProperties>());
		ThreadData = std::vector<ThreadInstanceCalculationData>(GetThreadCount(), ThreadInstanceCalculationData());

		AddCustomSaveFunction(std::bind_front(&LitRenderSystem::ComposeInstances, this));
	};

	void PreUpdate(const int thread, const double deltaTime) override
	{
		int threadCount = GetThreadCount();
		for (auto& instanceVector : InstanceThreadVectors)
		{
			instanceVector.clear();
		}

		for (size_t i = 0; i < BuiltInstances.size(); i++)
		{
			auto& builtInstance = BuiltInstances[i];
			size_t size = builtInstance.ModelMatrices.size();
			if (builtInstance.ModelMatrices.capacity() - size > 100)
			{
				builtInstance.ModelMatrices.shrink_to_fit();
				builtInstance.ModelMatrices.clear();
			}
			else { builtInstance.ModelMatrices.clear(); }

			for (auto& instanceVector : InstanceThreadVectors)
			{
				instanceVector.emplace_back(InstanceProperties(builtInstance.Material, std::vector<glm::mat4>()));
				instanceVector[i].ModelMatrices.reserve(size / threadCount * 2);
			}
		}
		for (auto& data : ThreadData)
		{
			data.previousInstanceIndex = 0;
		}
	};

	void Update(const int thread, const double deltaTime) override
	{
		CameraControllerSystemData csData = GetSystemData(&CameraSystemDataStorage);
		CurrentCameraComponent = GetEntityComponent(csData.MainCamera, CameraEntityType, &CameraEntityComponents);

		// Compose instances after all entities have been processed.
	};

	void PostUpdate(const int thread, const double deltaTime) override
	{
		RenderInstanced();
	};

	void UpdatePerEntity(const int thread, const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent tc = GetCurrentEntityComponent(thread, &TransformComponents);
		MaterialComponentLit mc = GetCurrentEntityComponent(thread, &MaterialComponents);
		ModelMatrixComponent matrix = GetCurrentEntityComponent(thread, &ModelMatrixComponents);
		size_t& previousInstanceIndex = ThreadData[thread].previousInstanceIndex;
		std::vector<InstanceProperties>& threadInstance = InstanceThreadVectors[thread];


		glm::mat4 modelMatrix = matrix.ModelMatrix;


		if (threadInstance.size() == 0)
		{
			InstanceProperties newInstances;
			newInstances.Material = mc;
			newInstances.ModelMatrices = std::vector<glm::mat4>();
			threadInstance.emplace_back(newInstances);
			threadInstance[0].ModelMatrices.emplace_back(modelMatrix);
			previousInstanceIndex = 0;
			return;
		}

		InstanceProperties& previousInstance = threadInstance[previousInstanceIndex];

		if (AreMaterialComponentsEqual(previousInstance.Material, mc))
		{
			threadInstance[previousInstanceIndex].ModelMatrices.emplace_back(modelMatrix);
			return;
		}

		size_t correctInstance = -1;
		for (size_t index = 0; index < threadInstance.size(); index++)
		{
			if (index == previousInstanceIndex) { continue; }
			if (AreMaterialComponentsEqual(threadInstance[index].Material, mc))
			{
				correctInstance = index;
				break;
			}
		}
		if (correctInstance == -1)
		{
			threadInstance.emplace_back(InstanceProperties(mc, {}));
			previousInstanceIndex = threadInstance.size() -1;
			threadInstance[previousInstanceIndex].ModelMatrices.emplace_back(modelMatrix);
			previousInstanceIndex = previousInstanceIndex;
			return;
		}

		threadInstance[correctInstance].ModelMatrices.emplace_back(modelMatrix);
		previousInstanceIndex = correctInstance;
		
	}

	void ComposeInstances()
	{
		// Manually save results from threaded calculations
		for (size_t i = 0; i < InstanceThreadVectors.size(); i++)
		{
			auto& instanceVector = InstanceThreadVectors[i];
			for (const auto& threadInstance : instanceVector)
			{
				bool found = false;
				for (auto& builtInstance : BuiltInstances)
				{
					if (AreMaterialComponentsEqual(threadInstance.Material, builtInstance.Material))
					{
						builtInstance.ModelMatrices.reserve(builtInstance.ModelMatrices.size() + threadInstance.ModelMatrices.size());
						std::ranges::copy(threadInstance.ModelMatrices, std::back_inserter(builtInstance.ModelMatrices));
						found = true;
						break;
					}
				}
				if (!found)
				{
					BuiltInstances.push_back(threadInstance);
				}
			}
		}
		for (size_t i = BuiltInstances.size(); i > 0;)
		{
			i--;
			auto& instance = BuiltInstances[i];
			if (instance.ModelMatrices.empty())
			{
				BuiltInstances.erase(BuiltInstances.begin() + i);
			}
		}
	}

	void RenderInstanced() const
	{
		// Update Camera locations
		auto projection = CurrentCameraComponent.Projection;
		auto view = CurrentCameraComponent.View;
		// Invert the Y direction on projection matrix so that we are more similar to opengl axis
		projection[1][1] *= -1;
		CameraData cameraData =
		{
			.view = view,
			.proj = projection,
			.viewproj = projection * view,
		};

		LitLightingData lighting = Resources->GlobalLightingData;
		LitLightingDataInShader  lightingData =
		{
			.ambientLightColor = glm::vec4(lighting.ambientLightColor, lighting.ambientLightStrength),
			.directionalLightColor = glm::vec4(lighting.directionalLightColor, lighting.directionalLightStrength),
			.directionalLightDir = glm::vec4(lighting.directionalLightDir, 0.0f)
		};



		RenderPipeline UsedPipeline = MainRenderPipelineManager->GetEngineRenderPipeline(EngineRenderPipelines::Lit);

		for (const auto& instance : BuiltInstances)
		{
			MaterialComponentLit materialComponent = instance.Material;

			TextureOptions textureOptions =
			{
				.color = materialComponent.Color,
				.textureRepeat = materialComponent.TextureRepeat,
			};

			auto meshData = MainMeshLoader->GetMeshData(materialComponent.RenderMesh);
			auto meshBuffers = MainMeshLoader->GetMeshBuffers(materialComponent.RenderMesh);

			// Allocate buffer data to gpu //

			// Allocate buffer for camera data.
			AllocatedBuffer currentCameraDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			MainAllocationHandler->WriteToBuffer(currentCameraDataBuffer, &cameraData);
			Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
				MainAllocationHandler->DestroyBuffer(currentCameraDataBuffer);
				});

			// Allocate buffer for lighting data.
			AllocatedBuffer lightingDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(LitLightingData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			MainAllocationHandler->WriteToBuffer(lightingDataBuffer, &lightingData);
			Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
				MainAllocationHandler->DestroyBuffer(lightingDataBuffer);
				});

			// Allocate buffer for texture options.
			AllocatedBuffer textureOptionsBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(TextureOptions), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
			MainAllocationHandler->WriteToBuffer(textureOptionsBuffer, &textureOptions);
			Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
				MainAllocationHandler->DestroyBuffer(textureOptionsBuffer);
				});

			GPUModelMatrixBuffer matrixBuffer = MainAllocationHandler->CreateModelMatrixBuffer(instance.ModelMatrices);
			Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
				MainAllocationHandler->DestroyBuffer(matrixBuffer.matrixBuffer);
				});

			// Allocate descriptor sets //

			// Create a descriptor set for camera info
			VkDescriptorSet CameraDataDescriptorSet = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.CameraDataDescriptorSetLayout);
			{
				DescriptorWriter writer;
				writer.WriteBuffer(0, currentCameraDataBuffer.buffer, sizeof(CameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
				writer.UpdateSet(Resources->vkDevice, CameraDataDescriptorSet);
			}

			// Create a descriptor set for lighting data
			VkDescriptorSet LightingDataDescriptorSet = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.LightingDataDescriptorSetLayout);
			{
				DescriptorWriter writer;
				writer.WriteBuffer(0, lightingDataBuffer.buffer, sizeof(LitLightingData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
				writer.UpdateSet(Resources->vkDevice, LightingDataDescriptorSet);
			}

			// Create a descriptor set for textures
			VkDescriptorSet TextureDataDescriptorSetLayout = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.TextureDataDescriptorSetLayout);
			{
				DescriptorWriter writer;
				auto diffuseTexture = MainTextureLoader->GetTexture(materialComponent.Diffuse);
				auto normalTexture = MainTextureLoader->GetTexture(materialComponent.Normal);
				writer.WriteImage(0, diffuseTexture.imageView, Resources->DefaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.WriteImage(1, normalTexture.imageView, Resources->DefaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
				writer.WriteBuffer(2, textureOptionsBuffer.buffer, sizeof(TextureOptions), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
				writer.UpdateSet(Resources->vkDevice, TextureDataDescriptorSetLayout);
			}

			// Rendering commands //

			vkCmdBindPipeline(Resources->CurrentCMD, VK_PIPELINE_BIND_POINT_GRAPHICS, UsedPipeline.Pipeline);

			// Set dynamic viewport and scissor
			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = Resources->vkDrawExtent.width;
			viewport.height = Resources->vkDrawExtent.height;
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;

			vkCmdSetViewport(Resources->CurrentCMD, 0, 1, &viewport);

			VkRect2D scissor = {};
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = Resources->vkDrawExtent.width;
			scissor.extent.height = Resources->vkDrawExtent.height;

			vkCmdSetScissor(Resources->CurrentCMD, 0, 1, &scissor);

			int indicesCount = meshData->indices.size();

			vkCmdBindIndexBuffer(Resources->CurrentCMD, meshBuffers->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			std::vector<VkDescriptorSet> FrameDescriptorSets;
			FrameDescriptorSets.push_back(CameraDataDescriptorSet);
			FrameDescriptorSets.push_back(LightingDataDescriptorSet);
			FrameDescriptorSets.push_back(TextureDataDescriptorSetLayout);
			vkCmdBindDescriptorSets(Resources->CurrentCMD, VK_PIPELINE_BIND_POINT_GRAPHICS, UsedPipeline.Layout, 0, 3, FrameDescriptorSets.data(), 0, nullptr);


			int instances = instance.ModelMatrices.size();

			InstancedPushConstants push_constants;
			push_constants.vertexBufferAddress = meshBuffers->vertexBufferAddress;
			push_constants.modelMatrixBufferAddress = matrixBuffer.matrixBufferAddress;

			vkCmdPushConstants(Resources->CurrentCMD, UsedPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(InstancedPushConstants), &push_constants);
			vkCmdDrawIndexed(Resources->CurrentCMD, indicesCount, instances, 0, 0, 0);

		}
	}

	
	void Destroy() override {};


	bool AreMaterialComponentsEqual(const MaterialComponentLit& c1, const MaterialComponentLit& c2)
	{
		if (c1.Diffuse != c2.Diffuse) { return false; }
		if (c1.Normal != c1.Normal) { return false; }
		if (c1.RenderMesh != c2.RenderMesh) { return false; }
		if (c1.TextureRepeat != c2.TextureRepeat) { return false; }
		if (c1.Color != c2.Color) { return false; }

		return true;
	}


private:
	RenderCore* EngineRenderCore;
	RenderCoreResources* Resources;
	MeshLoader* MainMeshLoader;
	TextureLoader* MainTextureLoader;
	RenderPipelineManager* MainRenderPipelineManager;
	AllocationHandler* MainAllocationHandler;

	EntityType CameraEntityType;
	CameraComponent CurrentCameraComponent;

	// Component Storages
	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<MaterialComponentLit> MaterialComponents;
	ReadOnlyComponentStorage<ModelMatrixComponent> ModelMatrixComponents;
	ReadOnlyAlwaysFetchedStorage<CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<CameraControllerSystemData> CameraSystemDataStorage;
	
	// Instance calculation
	std::vector<ThreadInstanceCalculationData> ThreadData;
	std::vector<std::vector<InstanceProperties>> InstanceThreadVectors;
	std::vector<InstanceProperties> BuiltInstances;
};