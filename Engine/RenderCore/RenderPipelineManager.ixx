module;
#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

export module RenderCore.RenderPipelineManager;

import RenderCore.Resources;
import RenderCore.RaytracingResources;
import RenderCore.VulkanInitializers;
import RenderCore.VulkanErrorLogger;
import RenderCore.VulkanDescriptors;
import RenderCore.PipelineBuilder;
import RenderCore.AllocationHandler;
import RenderCore.ShaderLoader;

import <array>;
import <vector>;

export struct RenderPipeline
{
	VkPipelineLayout Layout = nullptr;
	VkPipeline Pipeline = nullptr;
	
	std::vector<VkDescriptorSetLayout> DescriptorSets;

	VkDescriptorSetLayout CameraDataDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout ModelDataDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout LightingDataDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout TextureDataDescriptorSetLayout = nullptr;
};

export struct RaytracingPipeline
{
	std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> ShaderGroups;
	VkPipelineLayout Layout = nullptr;
	VkPipeline Pipeline = nullptr;

	std::vector<VkDescriptorSetLayout> DescriptorSets;
	VkDescriptorSetLayout RayGenDescriptorSetLayout;
	VkDescriptorSetLayout TLASDescriptorSetLayout;
	VkDescriptorSetLayout LightingDataDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout InstanceDataDescriptorSetLayout;
	VkDescriptorSetLayout ModelDataDescriptorSetLayout = nullptr;
	VkDescriptorSetLayout TextureDataDescriptorSetLayout = nullptr;

};


export enum class EngineRenderPipelines { GradientCompute = 0 };
export enum class EngineRTPipelines { PBR = 0 };
export enum class EngineRTStageIndices {RayGen = 0, Miss = 1, Shadow = 2, ClosestHit = 3, ShaderGroupCount = 4};


export class RenderPipelineManager
{
	friend class RenderCore;

public:

	RenderPipelineManager(ShaderLoader* shaderLoader, RenderCoreResources* resources, RaytracingResources* rtResources) :
	MainShaderLoader(shaderLoader),
	CoreResources(resources),
	RtResources(rtResources)
	{}
	~RenderPipelineManager() {}


	RenderPipelineID GetEngineRenderPipelineID(EngineRenderPipelines pipeline)
	{
		return static_cast<int>(pipeline);
	}

	RenderPipeline GetEngineRenderPipeline(RenderPipelineID id)
	{
		return Pipelines[id];
	}

	RenderPipeline GetEngineRenderPipeline(EngineRenderPipelines pipeline)
	{
		return Pipelines[static_cast<int>(pipeline)];
	}

	RaytracingPipeline GetEngineRtPipeline(EngineRTPipelines pipeline)
	{
		return RaytracingPipelines[static_cast<int>(pipeline)];
	}

protected:

	void InitEngineRenderPipelines()
	{
		for (int i = static_cast<int>(EngineRenderPipelines::GradientCompute); i <= static_cast<int>(EngineRenderPipelines::GradientCompute); i++)
		{
			Pipelines.push_back(RenderPipeline());
		}

		for (int i = static_cast<int>(EngineRTPipelines::PBR); i <= static_cast<int>(EngineRTPipelines::PBR); i++)
		{
			RaytracingPipelines.push_back(RaytracingPipeline());
		}

		InitGradientComputeDescriptors();
		InitGradientComputePipeline();

		InitRaytracingDescriptors();
		InitRaytracingPipeline();
	}


	void InitGradientComputeDescriptors()
	{
		RenderPipeline& GradientComputePipeline = Pipelines[static_cast<int>(EngineRenderPipelines::GradientCompute)];

		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		GradientComputePipeline.DescriptorSets.push_back(builder.Build(CoreResources->vkDevice, VK_SHADER_STAGE_COMPUTE_BIT));
		PipelinesDeletionQueue.push_back([&]()
			{
				for (auto ds : GradientComputePipeline.DescriptorSets)
				{
					vkDestroyDescriptorSetLayout(CoreResources->vkDevice, ds, nullptr);
				}
			}
		);
	}

	void InitGradientComputePipeline()
	{
		RenderPipeline& gradientPipeline = Pipelines[static_cast<int>(EngineRenderPipelines::GradientCompute)];

		VkShaderModule gradientShader = MainShaderLoader->GetShaderModuleById(static_cast<int>(EngineComputeShaders::Gradient));


		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(ExtraPushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkPipelineLayoutCreateInfo computeLayout{};
		computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		computeLayout.pNext = nullptr;
		computeLayout.pSetLayouts = gradientPipeline.DescriptorSets.data();
		computeLayout.setLayoutCount = 1;

		computeLayout.pPushConstantRanges = &pushConstant;
		computeLayout.pushConstantRangeCount = 1;

		VK_CHECK(vkCreatePipelineLayout(CoreResources->vkDevice, &computeLayout, nullptr, &gradientPipeline.Layout));


		// Layout code
		VkPipelineShaderStageCreateInfo stageinfo{};
		stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageinfo.pNext = nullptr;
		stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stageinfo.module = gradientShader;
		stageinfo.pName = "main";

		VkComputePipelineCreateInfo computePipelineCreateInfo{};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
		computePipelineCreateInfo.layout = gradientPipeline.Layout;
		computePipelineCreateInfo.stage = stageinfo;

		VK_CHECK(vkCreateComputePipelines(CoreResources->vkDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradientPipeline.Pipeline));

		// Push pipelines to the deletion queue
		PipelinesDeletionQueue.push_back([=]() {
			vkDestroyPipelineLayout(CoreResources->vkDevice, gradientPipeline.Layout, nullptr);
			vkDestroyPipeline(CoreResources->vkDevice, gradientPipeline.Pipeline, nullptr);
			});
	}

	void InitRaytracingPipeline()
	{
		RaytracingPipeline& raytracingPipeline = RaytracingPipelines[static_cast<int>(EngineRTPipelines::PBR)];

		// Shader modules
		VkShaderModule rayGenShader = MainShaderLoader->GetRaytracingShaderModuleById(static_cast<int>(EngineRaytracingShaders::RayGen));
		VkShaderModule rayCHitShader = MainShaderLoader->GetRaytracingShaderModuleById(static_cast<int>(EngineRaytracingShaders::RayClosestHit));
		VkShaderModule rayMiss = MainShaderLoader->GetRaytracingShaderModuleById(static_cast<int>(EngineRaytracingShaders::RayMiss));
		VkShaderModule rayShadow = MainShaderLoader->GetRaytracingShaderModuleById(static_cast<int>(EngineRaytracingShaders::RayShadow));

		// Shader stages
		VkPipelineShaderStageCreateInfo stageRayGen { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageRayGen.pName = "main";  // All the same entry point
		stageRayGen.module = rayGenShader;
		stageRayGen.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		raytracingPipeline.ShaderStages.push_back(stageRayGen);

		VkPipelineShaderStageCreateInfo stageRayMiss{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageRayMiss.pName = "main";  // All the same entry point
		stageRayMiss.module = rayMiss;
		stageRayMiss.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		raytracingPipeline.ShaderStages.push_back(stageRayMiss);

		VkPipelineShaderStageCreateInfo stageRayShadow{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageRayShadow.pName = "main";  // All the same entry point
		stageRayShadow.module = rayShadow;
		stageRayShadow.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
		raytracingPipeline.ShaderStages.push_back(stageRayShadow);

		VkPipelineShaderStageCreateInfo stageRayClosestHit{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageRayClosestHit.pName = "main";  // All the same entry point
		stageRayClosestHit.module = rayCHitShader;
		stageRayClosestHit.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		raytracingPipeline.ShaderStages.push_back(stageRayClosestHit);

		// Shader groups - Raygen shaders
		VkRayTracingShaderGroupCreateInfoKHR groupRayGen{};
		groupRayGen.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupRayGen.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupRayGen.anyHitShader = VK_SHADER_UNUSED_KHR;
		groupRayGen.closestHitShader = VK_SHADER_UNUSED_KHR;
		groupRayGen.generalShader = static_cast<uint32_t>(EngineRTStageIndices::RayGen);
		groupRayGen.intersectionShader = VK_SHADER_UNUSED_KHR;
		raytracingPipeline.ShaderGroups.push_back(groupRayGen);

		// Shader groups - Miss shaders
		VkRayTracingShaderGroupCreateInfoKHR groupMiss{};
		groupMiss.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupMiss.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		groupMiss.anyHitShader = VK_SHADER_UNUSED_KHR;
		groupMiss.closestHitShader = VK_SHADER_UNUSED_KHR;
		groupMiss.generalShader = static_cast<uint32_t>(EngineRTStageIndices::Miss);
		groupMiss.intersectionShader = VK_SHADER_UNUSED_KHR;
		raytracingPipeline.ShaderGroups.push_back(groupMiss);
		groupMiss.generalShader = static_cast<uint32_t>(EngineRTStageIndices::Shadow);
		raytracingPipeline.ShaderGroups.push_back(groupMiss);

		// Shader groups - Hit shaders
		VkRayTracingShaderGroupCreateInfoKHR groupClosestHit{};
		groupClosestHit.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		groupClosestHit.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		groupClosestHit.anyHitShader = VK_SHADER_UNUSED_KHR;
		groupClosestHit.closestHitShader = static_cast<uint32_t>(EngineRTStageIndices::ClosestHit);
		groupClosestHit.generalShader = VK_SHADER_UNUSED_KHR;
		groupClosestHit.intersectionShader = VK_SHADER_UNUSED_KHR;
		raytracingPipeline.ShaderGroups.push_back(groupClosestHit);


		// Layout creation

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(RaytracingPushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

		VkPipelineLayoutCreateInfo raytracingLayout{};
		raytracingLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		raytracingLayout.pNext = nullptr;
		raytracingLayout.pSetLayouts = raytracingPipeline.DescriptorSets.data();
		raytracingLayout.setLayoutCount = static_cast<uint32_t>(raytracingPipeline.DescriptorSets.size());

		raytracingLayout.pPushConstantRanges = &pushConstant;
		raytracingLayout.pushConstantRangeCount = 1;

		VK_CHECK(vkCreatePipelineLayout(CoreResources->vkDevice, &raytracingLayout, nullptr, &raytracingPipeline.Layout));


		// Pipeline creation

		// Assemble the shader stages and recursion depth info into the ray tracing pipeline
		VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
		rayPipelineInfo.stageCount = static_cast<uint32_t>(raytracingPipeline.ShaderStages.size());  // Stages are shaders
		rayPipelineInfo.pStages = raytracingPipeline.ShaderStages.data();

		rayPipelineInfo.groupCount = static_cast<uint32_t>(raytracingPipeline.ShaderGroups.size());
		rayPipelineInfo.pGroups = raytracingPipeline.ShaderGroups.data();
		rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
		rayPipelineInfo.layout = raytracingPipeline.Layout;

		CoreResources->DispatchTable.fp_vkCreateRayTracingPipelinesKHR(CoreResources->vkDevice, {}, {}, 1, &rayPipelineInfo, nullptr, &raytracingPipeline.Pipeline);


		// Push pipelines to the deletion queue
		PipelinesDeletionQueue.push_back([=]() {
			vkDestroyPipelineLayout(CoreResources->vkDevice, raytracingPipeline.Layout, nullptr);
			vkDestroyPipeline(CoreResources->vkDevice, raytracingPipeline.Pipeline, nullptr);
			});
	}

	void InitRaytracingDescriptors()
	{
		RaytracingPipeline& raytracingPipeline = RaytracingPipelines[static_cast<int>(EngineRTPipelines::PBR)];
		DescriptorLayoutBuilder builder;

		const VkDescriptorBindingFlagsEXT extFlags =
			VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;

		// Set 0 - Result Image
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		raytracingPipeline.RayGenDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_RAYGEN_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.RayGenDescriptorSetLayout);
		builder.Clear();

		// Set 1 - Top Level Acceleration Structure + Camera Data
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		raytracingPipeline.TLASDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.TLASDescriptorSetLayout);
		builder.Clear();

		// Set 2 - Lighting Data
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		raytracingPipeline.LightingDataDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.LightingDataDescriptorSetLayout);
		builder.Clear();

		// Set 3 - Instance Data
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		raytracingPipeline.InstanceDataDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.InstanceDataDescriptorSetLayout);
		builder.Clear();

		// Set 4 - Model Data
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256);
		raytracingPipeline.ModelDataDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.ModelDataDescriptorSetLayout);
		builder.Clear();

		// Set 5 - Texture Data
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_SAMPLER);						// Sampler
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// Albedo
		builder.AddBinding(2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// AO
		builder.AddBinding(3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// Depth
		builder.AddBinding(4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// Metallic
		builder.AddBinding(5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// Normal
		builder.AddBinding(6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256);		// Roughness
		raytracingPipeline.TextureDataDescriptorSetLayout = builder.BuildWithExtFlags(CoreResources->vkDevice, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, extFlags);
		raytracingPipeline.DescriptorSets.push_back(raytracingPipeline.TextureDataDescriptorSetLayout);

		PipelinesDeletionQueue.push_back([&]()
		{
			for (auto ds : raytracingPipeline.DescriptorSets)
			{
				vkDestroyDescriptorSetLayout(CoreResources->vkDevice, ds, nullptr);
			}
		});
	}

	void CleanEngineRenderPipelines()
	{
		PipelinesDeletionQueue.flush();
	}
	

	std::vector<RenderPipeline> Pipelines;
	std::vector<RaytracingPipeline> RaytracingPipelines;

	DeletionQueue PipelinesDeletionQueue;

	ShaderLoader* MainShaderLoader;
	RenderCoreResources* CoreResources;
	RaytracingResources* RtResources;
};
