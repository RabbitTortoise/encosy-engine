module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>

export module Systems.VulkanRenderSystem;

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


export class VulkanRenderSystem : public System
{
	friend class SystemManager;

	struct EntityData
	{
		CameraControllerSystemData CameraSystemData;
		std::vector<std::span<TransformComponent const>> TransformComponents;
		std::vector<std::span<MaterialComponent const>> MaterialComponents;
		std::span<CameraComponent const> CameraComponents;
	};
	EntityData HandledData;
	

public:
	VulkanRenderSystem(RenderCore* engineRenderCore) : System()
	{ 
		EngineRenderCore = engineRenderCore;
		MainMeshLoader = EngineRenderCore->GetMeshLoader();
		MainTextureLoader = EngineRenderCore->GetTextureLoader();
		MainRenderPipelineManager = EngineRenderCore->GetRenderPipelineManager();
		Resources = EngineRenderCore->GetRenderCoreResources();
		MainAllocationHandler = EngineRenderCore->GetAllocationHandler();
	}
	~VulkanRenderSystem() {}

protected:
	void Init() override
	{
		Type = SystemType::RenderSystem;
		SystemQueueIndex = 1000;

		auto info = WorldEntityManager->GetEntityTypeInfo("CameraEntity");

		AddSystemDataForReading(&HandledData.CameraSystemData);
		AddWantedComponentDataForReading(&HandledData.TransformComponents);
		AddWantedComponentDataForReading(&HandledData.MaterialComponents);
		AddAlwaysFetchedEntitiesForReading(info.TypeID, &HandledData.CameraComponents);

		CameraEntityType = WorldEntityManager->GetEntityTypeInfo("CameraEntity").TypeID;

	};

	void PreUpdate(float deltaTime) override {};
	void Update(float deltaTime) override 
	{
		for (size_t i = 0; i < HandledData.CameraComponents.size(); i++)
		{
			auto id = WorldEntityManager->GetEntityIdFromComponentIndex(CameraEntityType, i);
			if (id == HandledData.CameraSystemData.MainCamera.GetID())
			{
				CurrentCameraComponent = HandledData.CameraComponents[i];
				break;
			}
		}



	};

	void UpdatePerEntity(float deltaTime, Entity entity, size_t vectorIndex, size_t spanIndex) override
	{
		TransformComponent transformComponent = HandledData.TransformComponents[vectorIndex][spanIndex];
		MaterialComponent materialComponent = HandledData.MaterialComponents[vectorIndex][spanIndex];
		
		glm::mat4 modelMatrix = MatrixCalculations::CalculateModelMatrix(transformComponent);

		// For now only process entities with unlit pipeline
		if (static_cast<int>(EngineRenderPipelines::Unlit) == materialComponent.UsedRenderPipeline)
		{
			// Update Camera locations
			auto projection = CurrentCameraComponent.ProjectionMatrix;
			auto view = CurrentCameraComponent.Orientation;
			// Invert the Y direction on projection matrix so that we are more similar to opengl axis
			projection[1][1] *= -1;
			CameraData cameraData =
			{
				.view = view,
				.proj = projection,
				.viewproj = projection * view,
			};

			RenderPipeline UsedPipeline = MainRenderPipelineManager->GetEngineRenderPipeline(materialComponent.UsedRenderPipeline);
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
				auto usedTexture = MainTextureLoader->GetTexture(materialComponent.Texture_Albedo);
				writer.WriteImage(0, usedTexture.imageView, Resources->DefaultSamplerNearest, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
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
	}

	void PostUpdate(float deltaTime) override {};
	void Destroy() override {};

private:
	RenderCore* EngineRenderCore;
	RenderCoreResources* Resources;
	MeshLoader* MainMeshLoader;
	TextureLoader* MainTextureLoader;
	RenderPipelineManager* MainRenderPipelineManager;
	AllocationHandler* MainAllocationHandler;

	EntityTypeID CameraEntityType;
	CameraComponent CurrentCameraComponent;
};