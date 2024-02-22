module;
#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

export module RenderCore.RenderPipelineManager;

import RenderCore.Resources;
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

export enum class EngineRenderPipelines {Unlit = 0, Lit = 1, GradientCompute = 2};


export class RenderPipelineManager
{
	friend class RenderCore;

public:

	RenderPipelineManager(ShaderLoader* shaderLoader, RenderCoreResources* resources) : MainShaderLoader(shaderLoader), Resources(resources) {}
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

protected:

	void InitEngineRenderPipelines()
	{
		for (int i = static_cast<int>(EngineRenderPipelines::Unlit); i <= static_cast<int>(EngineRenderPipelines::GradientCompute); i++)
		{
			Pipelines.push_back(RenderPipeline());
		}

		InitUnlitDescriptors();
		InitUnlitPipeline();

		InitLitDescriptors();
		InitLitPipeline();

		InitGradientComputeDescriptors();
		InitGradientComputePipeline();


	}

	void InitUnlitDescriptors()
	{
		RenderPipeline& UnlitPipeline = Pipelines[static_cast<int>(EngineRenderPipelines::Unlit)];
		DescriptorLayoutBuilder builder;

		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		UnlitPipeline.CameraDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_VERTEX_BIT);
		UnlitPipeline.DescriptorSets.push_back(UnlitPipeline.CameraDataDescriptorSetLayout);

		builder.Clear();
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		UnlitPipeline.ModelDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_VERTEX_BIT);
		UnlitPipeline.DescriptorSets.push_back(UnlitPipeline.ModelDataDescriptorSetLayout);

		builder.Clear();
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		UnlitPipeline.TextureDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_FRAGMENT_BIT);
		UnlitPipeline.DescriptorSets.push_back(UnlitPipeline.TextureDataDescriptorSetLayout);

		PipelinesDeletionQueue.push_back([&]()
			{
				for (auto ds : UnlitPipeline.DescriptorSets)
				{
					vkDestroyDescriptorSetLayout(Resources->vkDevice, ds, nullptr);
				}
			});
	}

	void InitLitDescriptors()
	{
		RenderPipeline& LitPipeline = Pipelines[static_cast<int>(EngineRenderPipelines::Lit)];
		DescriptorLayoutBuilder builder;

		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		LitPipeline.CameraDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_VERTEX_BIT);
		LitPipeline.DescriptorSets.push_back(LitPipeline.CameraDataDescriptorSetLayout);

		builder.Clear();
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		LitPipeline.LightingDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
		LitPipeline.DescriptorSets.push_back(LitPipeline.LightingDataDescriptorSetLayout);

		builder.Clear();
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		builder.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		LitPipeline.TextureDataDescriptorSetLayout = builder.Build(Resources->vkDevice, VK_SHADER_STAGE_FRAGMENT_BIT);
		LitPipeline.DescriptorSets.push_back(LitPipeline.TextureDataDescriptorSetLayout);

		PipelinesDeletionQueue.push_back([&]()
			{
				for (auto ds : LitPipeline.DescriptorSets)
				{
					vkDestroyDescriptorSetLayout(Resources->vkDevice, ds, nullptr);
				}
			});
	}

	void InitGradientComputeDescriptors()
	{
		RenderPipeline& GradientComputePipeline = Pipelines[static_cast<int>(EngineRenderPipelines::GradientCompute)];

		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		GradientComputePipeline.DescriptorSets.push_back(builder.Build(Resources->vkDevice, VK_SHADER_STAGE_COMPUTE_BIT));
		PipelinesDeletionQueue.push_back([&]()
			{
				for (auto ds : GradientComputePipeline.DescriptorSets)
				{
					vkDestroyDescriptorSetLayout(Resources->vkDevice, ds, nullptr);
				}
			}
		);
	}

	void InitUnlitPipeline()
	{
		RenderPipeline& UnlitPipeline = Pipelines[static_cast<int>(EngineRenderPipelines::Unlit)];

		VkShaderModule unlitVertexShader = MainShaderLoader->GetShaderModuleById(static_cast<int>(EngineVertexShaders::Unlit));
		VkShaderModule unlitFragShader = MainShaderLoader->GetShaderModuleById(static_cast<int>(EngineFragmentShaders::Unlit));

		VkPushConstantRange bufferRange{};
		bufferRange.offset = 0;
		bufferRange.size = sizeof(VertexBufferPushConstants);
		bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo unlit_layout_info = vkInit::Pipeline_Layout_CreateInfo();
		unlit_layout_info.pPushConstantRanges = &bufferRange;
		unlit_layout_info.pushConstantRangeCount = 1;
		unlit_layout_info.setLayoutCount = 3;
		unlit_layout_info.pSetLayouts = UnlitPipeline.DescriptorSets.data();

		VK_CHECK(vkCreatePipelineLayout(Resources->vkDevice, &unlit_layout_info, nullptr, &UnlitPipeline.Layout));

		// Build the pipeline
		PipelineBuilder pipelineBuilder;

		pipelineBuilder.vkPipelineLayout = UnlitPipeline.Layout;
		pipelineBuilder.SetShaders(unlitVertexShader, unlitFragShader);
		pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
		pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		pipelineBuilder.SetMultisampling_None();
		pipelineBuilder.DisableBlending();
		pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
		pipelineBuilder.SetColorAttachmentFormat(Resources->vkDrawImage.imageFormat);
		pipelineBuilder.SetDepthFormat(Resources->vkDepthImage.imageFormat);

		UnlitPipeline.Pipeline = pipelineBuilder.BuildPipeline(Resources->vkDevice);

		// Push pipelines to the deletion queue
		PipelinesDeletionQueue.push_back([=]() {
			vkDestroyPipelineLayout(Resources->vkDevice, UnlitPipeline.Layout, nullptr);
			vkDestroyPipeline(Resources->vkDevice, UnlitPipeline.Pipeline, nullptr);
			});
	}

	void InitLitPipeline()
	{
		RenderPipeline& LitPipeline = Pipelines[static_cast<int>(EngineRenderPipelines::Lit)];

		VkShaderModule litVertexShader = MainShaderLoader->GetShaderModuleById(static_cast<int>(EngineVertexShaders::Lit));
		VkShaderModule litFragShader = MainShaderLoader->GetShaderModuleById(static_cast<int>(EngineFragmentShaders::Lit));
		
		VkPushConstantRange bufferRange{};
		bufferRange.offset = 0;
		bufferRange.size = sizeof(InstancedPushConstants);
		bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo lit_layout_info = vkInit::Pipeline_Layout_CreateInfo();
		lit_layout_info.pPushConstantRanges = &bufferRange;
		lit_layout_info.pushConstantRangeCount = 1;
		lit_layout_info.setLayoutCount = 3;
		lit_layout_info.pSetLayouts = LitPipeline.DescriptorSets.data();

		VK_CHECK(vkCreatePipelineLayout(Resources->vkDevice, &lit_layout_info, nullptr, &LitPipeline.Layout));

		// Build the pipeline
		PipelineBuilder pipelineBuilder;

		pipelineBuilder.vkPipelineLayout = LitPipeline.Layout;
		pipelineBuilder.SetShaders(litVertexShader, litFragShader);
		pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
		pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
		pipelineBuilder.SetMultisampling_None();
		pipelineBuilder.DisableBlending();
		pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
		pipelineBuilder.SetColorAttachmentFormat(Resources->vkDrawImage.imageFormat);
		pipelineBuilder.SetDepthFormat(Resources->vkDepthImage.imageFormat);

		LitPipeline.Pipeline = pipelineBuilder.BuildPipeline(Resources->vkDevice);

		// Push pipelines to the deletion queue
		PipelinesDeletionQueue.push_back([=]() {
			vkDestroyPipelineLayout(Resources->vkDevice, LitPipeline.Layout, nullptr);
			vkDestroyPipeline(Resources->vkDevice, LitPipeline.Pipeline, nullptr);
			});
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

		VK_CHECK(vkCreatePipelineLayout(Resources->vkDevice, &computeLayout, nullptr, &gradientPipeline.Layout));


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

		VK_CHECK(vkCreateComputePipelines(Resources->vkDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradientPipeline.Pipeline));

		// Push pipelines to the deletion queue
		PipelinesDeletionQueue.push_back([=]() {
			vkDestroyPipelineLayout(Resources->vkDevice, gradientPipeline.Layout, nullptr);
			vkDestroyPipeline(Resources->vkDevice, gradientPipeline.Pipeline, nullptr);
			});
	}

	void CleanEngineRenderPipelines()
	{
		PipelinesDeletionQueue.flush();
	}
	

	std::vector<RenderPipeline> Pipelines;
	DeletionQueue PipelinesDeletionQueue;

	ShaderLoader* MainShaderLoader;
	RenderCoreResources* Resources;
};
