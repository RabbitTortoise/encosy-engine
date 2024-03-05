module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Systems.LitRenderSystem;

import ECS.System;
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


struct InstanceProperties
{
	MaterialComponentLit Material = {};
	std::vector<glm::mat4> ModelMatrices;
};

export class LitRenderSystem : public System
{
	friend class SystemManager;

public:
	LitRenderSystem(RenderCore* engineRenderCore) : System()
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
		SystemQueueIndex = 1000;

		auto cameraEntityInfo = WorldEntityManager->GetEntityTypeInfo("CameraEntity");

		AddSystemDataForReading(&CameraSystemDataStorage);
		AddWantedComponentDataForReading(&TransformComponents);
		AddWantedComponentDataForReading(&MaterialComponents);
		AddWantedComponentDataForReading(&ModelMatrixComponents);
		AddAlwaysFetchedEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);
		
		CameraEntityType = cameraEntityInfo.Type;

	};

	void PreUpdate(const double deltaTime) override {};
	void Update(const double deltaTime) override 
	{
		CameraControllerSystemData csData = GetSystemData(&CameraSystemDataStorage);
		CurrentCameraComponent = GetEntityComponent(csData.MainCamera, CameraEntityType, &CameraEntityComponents);

		previousInstanceIndex = -1;
		DifferentInstances = std::vector<InstanceProperties>();
		ExecutePerEntity(std::bind_front(&LitRenderSystem::InstanceBuilder, this));

		RenderInstanced();
	};

	void InstanceBuilder(const double deltaTime, Entity entity, EntityType entityType)
	{
		TransformComponent tc = GetCurrentEntityComponent(&TransformComponents);
		MaterialComponentLit mc = GetCurrentEntityComponent(&MaterialComponents);
		ModelMatrixComponent matrix = GetCurrentEntityComponent(&ModelMatrixComponents);
		
		glm::mat4 modelMatrix = matrix.ModelMatrix;


		if (previousInstanceIndex == -1)
		{
			InstanceProperties newInstances;
			newInstances.Material = mc;
			newInstances.ModelMatrices = std::vector<glm::mat4>();
			DifferentInstances.push_back(newInstances);
			DifferentInstances[0].ModelMatrices.push_back(modelMatrix);
			previousInstanceIndex = 0;
			return;
		}

		InstanceProperties previousInstance = DifferentInstances[previousInstanceIndex];

		if (AreMaterialComponentsEqual(previousInstance.Material, mc))
		{
			DifferentInstances[previousInstanceIndex].ModelMatrices.push_back(modelMatrix);
			return;
		}

		size_t correctInstance = -1;
		for (size_t index = 0; index < DifferentInstances.size(); index++)
		{
			if (index == previousInstanceIndex) { continue; }
			if (AreMaterialComponentsEqual(DifferentInstances[index].Material, mc))
			{
				correctInstance = index;
				break;
			}
		}
		if (correctInstance == -1)
		{
			InstanceProperties newInstances = InstanceProperties();
			newInstances.Material = mc;
			newInstances.ModelMatrices = std::vector<glm::mat4>();
			DifferentInstances.push_back(newInstances);
			previousInstanceIndex += 1;
			DifferentInstances[previousInstanceIndex].ModelMatrices.push_back(modelMatrix);
			return;
		}
		else
		{
			DifferentInstances[correctInstance].ModelMatrices.push_back(modelMatrix);
			previousInstanceIndex = correctInstance;
		}
	}

	void RenderInstanced()
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

		for (const auto& instance : DifferentInstances)
		{
			MaterialComponentLit materialComponent = instance.Material;

			TextureOptions textureOptions =
			{
				.textureRepeat = materialComponent.TextureRepeat
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

	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override {};
	void PostUpdate(const double deltaTime) override {};
	void Destroy() override {};


	bool AreMaterialComponentsEqual(const MaterialComponentLit& c1, const MaterialComponentLit& c2)
	{
		if (c1.Diffuse != c2.Diffuse) { return false; }
		if (c1.Normal != c1.Normal) { return false; }
		if (c1.RenderMesh != c2.RenderMesh) { return false; }
		if (c1.TextureRepeat != c2.TextureRepeat) { return false; }

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
	size_t previousInstanceIndex = -1;
	std::vector<InstanceProperties> DifferentInstances;
};