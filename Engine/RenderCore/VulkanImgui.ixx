module;
#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

export module RenderCore.VulkanImgui;

import RenderCore.VulkanErrorLogger;
import RenderCore.Resources;
import RenderCore.VulkanInitializers;

import <format>;
import <string>;


export
class VulkanImgui
{
public:
	VulkanImgui(RenderCoreResources* resources)
	{
		Resources = resources;
	}
	~VulkanImgui() {}


	void InitImgui(int minAlloctionSize)
	{
		// 1: Create descriptor pool for IMGUI
		//  - the size of the pool is oversized, but it's copied from imgui demo itself.
		VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000;
		pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;

		VkDescriptorPool imguiPool;
		VK_CHECK(vkCreateDescriptorPool(Resources->vkDevice, &pool_info, nullptr, &imguiPool));

		// 2: initialize imgui library
		// This initializes the core structures of imgui
		ImGui::CreateContext();

		// This initializes imgui for SDL
		ImGui_ImplSDL3_InitForVulkan(Resources->MainWindow->GetWindow());

		VkPipelineRenderingCreateInfo dynamic_rendering_info = {};
		dynamic_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		dynamic_rendering_info.pNext = 0;
		dynamic_rendering_info.viewMask = 0;
		dynamic_rendering_info.colorAttachmentCount = 1;
		dynamic_rendering_info.pColorAttachmentFormats = &Resources->vkSwapchainImageFormat;
		dynamic_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		dynamic_rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;


		// This initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = Resources->vkInstance;
		init_info.PhysicalDevice = Resources->vkChosenGPU;
		init_info.Device = Resources->vkDevice;
		init_info.Queue = Resources->vkGraphicsSetupQueue;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.MinAllocationSize = minAlloctionSize;
		init_info.UseDynamicRendering = true;
		init_info.PipelineRenderingCreateInfo = dynamic_rendering_info;

		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();

		// Add the destroy the imgui created structures
		ImGuiDeletionQueue.push_back([=]() {
			ImGui_ImplVulkan_DestroyFontsTexture();
			vkDestroyDescriptorPool(Resources->vkDevice, imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			});

		Resources->MainWindow->SubscribeToEvents([&](SDL_Event e) {
			ImGui_ImplSDL3_ProcessEvent(&e);
			});
	}
	
	void UpdateImgui(size_t entityCount)
	{
		// Imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("Rendering Settings")) {
			ImGui::Text(std::format("FPS: {}", ImGui::GetIO().Framerate).c_str());
			ImGui::Text(std::format("Entities simulated: {}", entityCount).c_str());

			ImGui::SliderFloat("Render Scale", &Resources->RenderScale, 0.3f, 1.f);

			ImGui::Text("Gradient settings");

			ImGui::InputFloat4("Top", (float*)&Resources->BackgroundGradientData.data1);
			ImGui::InputFloat4("Bottom", (float*)&Resources->BackgroundGradientData.data2);

			ImGui::Text("Lighting settings");
			ImGui::SliderFloat("Ambient light strength", &Resources->GlobalLightingData.ambientLightStrength, 0.0f, 1.0f);

			ImGui::SliderFloat("Ambient light color R", &Resources->GlobalLightingData.ambientLightColor.r, 0.0f, 1.0f);
			ImGui::SliderFloat("Ambient light color G", &Resources->GlobalLightingData.ambientLightColor.g, 0.0f, 1.0f);
			ImGui::SliderFloat("Ambient light color B", &Resources->GlobalLightingData.ambientLightColor.b, 0.0f, 1.0f);

			ImGui::SliderFloat("Directional light strength", &Resources->GlobalLightingData.directionalLightStrength, 0.0f, 1.0f);

			ImGui::SliderFloat("Directional light direction X", &Resources->GlobalLightingData.directionalLightDir.x, -1.0f, 1.0f);
			ImGui::SliderFloat("Directional light direction Y", &Resources->GlobalLightingData.directionalLightDir.y, -1.0f, 1.0f);
			ImGui::SliderFloat("Directional light direction Z", &Resources->GlobalLightingData.directionalLightDir.z, -1.0f, 1.0f);

			ImGui::SliderFloat("Directional light color R", &Resources->GlobalLightingData.directionalLightColor.r, 0.0f, 1.0f);
			ImGui::SliderFloat("Directional light color G", &Resources->GlobalLightingData.directionalLightColor.g, 0.0f, 1.0f);
			ImGui::SliderFloat("Directional light color B", &Resources->GlobalLightingData.directionalLightColor.b, 0.0f, 1.0f);

		}

		// Imgui UI to test
		//ImGui::ShowDemoWindow();
		ImGui::End();
		// Make imgui calculate internal draw structures
		ImGui::Render();
	}

	void RenderImgui(VkCommandBuffer cmd, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = vkInit::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
		VkRenderingInfo renderInfo = vkInit::RenderingInfo(Resources->vkSwapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		vkCmdEndRendering(cmd);
	}

	void Cleanup()
	{
		ImGuiDeletionQueue.flush();

		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
	}

private:
	DeletionQueue ImGuiDeletionQueue;
	RenderCoreResources* Resources;
};