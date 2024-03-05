module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

export module Systems.UnlitRenderSystem;

import ECS.System;
import Components.TransformComponent;
import Components.MaterialComponent;
import Components.CameraComponent;
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

export class UnlitRenderSystem : public System
{
	friend class SystemManager;

public:
	UnlitRenderSystem(RenderCore* engineRenderCore) : System()
	{ 
		EngineRenderCore = engineRenderCore;
		MainMeshLoader = EngineRenderCore->GetMeshLoader();
		MainTextureLoader = EngineRenderCore->GetTextureLoader();
		MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();
		Resources = EngineRenderCore->GetRenderCoreResources();
		MainAllocationHandler = EngineRenderCore->GetAllocationHandler();
	}
	~UnlitRenderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		SystemQueueIndex = 1000;

		auto cameraEntityInfo = WorldEntityManager->GetEntityTypeInfo("CameraEntity");

		AddSystemDataForReading(&CameraSystemDataStorage);
		AddWantedComponentDataForReading(&TransformComponents);
		AddWantedComponentDataForReading(&MaterialComponents);
		AddAlwaysFetchedEntitiesForReading(cameraEntityInfo.Type, &CameraEntityComponents);

		CameraEntityType = cameraEntityInfo.Type;

	};

	void PreUpdate(const double deltaTime) override {};
	void Update(const double deltaTime) override 
	{
		CameraControllerSystemData csData = GetSystemData(&CameraSystemDataStorage);
		CurrentCameraComponent = GetEntityComponent(csData.MainCamera, CameraEntityType, &CameraEntityComponents);

	};

	void UpdatePerEntity(const double deltaTime, Entity entity, EntityType entityType) override
	{
		TransformComponent transformComponent = GetCurrentEntityComponent(&TransformComponents);
		MaterialComponentUnlit materialComponent = GetCurrentEntityComponent(&MaterialComponents);
		

		glm::mat4 modelMatrix = MatrixCalculations::CalculateModelMatrix(transformComponent);

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

		TextureOptions textureOptions =
		{
			.textureRepeat = materialComponent.TextureRepeat
		};

		RenderPipeline UsedPipeline = MainRenderPipelineManager->GetEngineRenderPipeline(EngineRenderPipelines::Unlit);
		auto meshData = MainMeshLoader->GetMeshData(materialComponent.RenderMesh);
		auto meshBuffers = MainMeshLoader->GetMeshBuffers(materialComponent.RenderMesh);

		// Allocate buffer for model matrix.
		AllocatedBuffer modelDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		MainAllocationHandler->WriteToBuffer(modelDataBuffer, &modelMatrix);
		Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(modelDataBuffer);
			});

		// Allocate buffer for camera data.
		AllocatedBuffer CurrentCameraDataBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(CameraData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		MainAllocationHandler->WriteToBuffer(CurrentCameraDataBuffer, &cameraData);
		Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(CurrentCameraDataBuffer);
			});

		// Allocate buffer for texture options.
		AllocatedBuffer TextureOptionsBuffer = MainAllocationHandler->CreateGPUBuffer(sizeof(TextureOptions), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		MainAllocationHandler->WriteToBuffer(TextureOptionsBuffer, &textureOptions);
		Resources->GetCurrentFrame().vkDeletionQueue.push_back([=, this]() {
			MainAllocationHandler->DestroyBuffer(TextureOptionsBuffer);
			});

		// Create a descriptor set for camera info
		VkDescriptorSet CameraDataDescriptorSet = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.CameraDataDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, CurrentCameraDataBuffer.buffer, sizeof(CameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.UpdateSet(Resources->vkDevice, CameraDataDescriptorSet);
		}

		// Create a descriptor set for model matrix
		VkDescriptorSet ModelDataDescriptorSet = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.ModelDataDescriptorSetLayout);
		{
			DescriptorWriter writer;
			writer.WriteBuffer(0, modelDataBuffer.buffer, sizeof(glm::mat4), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.UpdateSet(Resources->vkDevice, ModelDataDescriptorSet);
		}

		// Create a descriptor set for textures
		VkDescriptorSet TextureDataDescriptorSetLayout = Resources->GetCurrentFrame().vkFrameDescriptors.Allocate(Resources->vkDevice, UsedPipeline.TextureDataDescriptorSetLayout);
		{
			DescriptorWriter writer;
			auto usedTexture = MainTextureLoader->GetTexture(materialComponent.Diffuse);
			writer.WriteImage(0, usedTexture.imageView, Resources->DefaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteBuffer(1, TextureOptionsBuffer.buffer, sizeof(TextureOptions), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.UpdateSet(Resources->vkDevice, TextureDataDescriptorSetLayout);
		}

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


		VertexBufferPushConstants push_constants;
		push_constants.vertexBuffer = meshBuffers->vertexBufferAddress;

		vkCmdPushConstants(Resources->CurrentCMD, UsedPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VertexBufferPushConstants), &push_constants);
		vkCmdBindIndexBuffer(Resources->CurrentCMD, meshBuffers->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		std::vector<VkDescriptorSet> FrameDescriptorSets;
		FrameDescriptorSets.push_back(CameraDataDescriptorSet);
		FrameDescriptorSets.push_back(ModelDataDescriptorSet);
		FrameDescriptorSets.push_back(TextureDataDescriptorSetLayout);

		vkCmdBindDescriptorSets(Resources->CurrentCMD, VK_PIPELINE_BIND_POINT_GRAPHICS, UsedPipeline.Layout, 0, 3, FrameDescriptorSets.data(), 0, nullptr);
		vkCmdDrawIndexed(Resources->CurrentCMD, indicesCount, 1, 0, 0, 0);

	}

	void PostUpdate(const double deltaTime) override {};
	void Destroy() override {};

private:
	RenderCore* EngineRenderCore;
	RenderCoreResources* Resources;
	MeshLoader* MainMeshLoader;
	TextureLoader* MainTextureLoader;
	RenderPipelineManager* MainRenderPipelineManager;
	AllocationHandler* MainAllocationHandler;

	EntityType CameraEntityType;
	CameraComponent CurrentCameraComponent;

	ReadOnlyComponentStorage<TransformComponent> TransformComponents;
	ReadOnlyComponentStorage<MaterialComponentUnlit> MaterialComponents;
	ReadOnlyAlwaysFetchedStorage<CameraComponent> CameraEntityComponents;
	ReadOnlySystemDataStorage<CameraControllerSystemData> CameraSystemDataStorage;
};