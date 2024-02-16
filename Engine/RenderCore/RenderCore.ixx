module;
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>


#include <vma/vk_mem_alloc.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <array>

export module EncosyEngine.RenderCore;

import EncosyEngine.MatrixCalculations;
import EncosyEngine.WindowManager;

import RenderCore.Resources;
import RenderCore.VulkanInitializers;
import RenderCore.VulkanErrorLogger;

import RenderCore.VulkanImages;
import RenderCore.VulkanDescriptors;
import RenderCore.AllocationHandler;
import RenderCore.MeshLoader;
import RenderCore.TextureLoader;
import RenderCore.ShaderLoader;
import RenderCore.RenderPipelineManager;


import <memory>;
import <optional>;
import <string>;
import <span>;
import <array>;
import <functional>;
import <deque>;
import <filesystem>;
import <iostream>;


export class RenderCore
{
	friend class EncosyApplication;

public:

	RenderCore(){}
	~RenderCore(){}

	MeshLoader* GetMeshLoader() { return MainMeshLoader.get(); }
	TextureLoader* GetTextureLoader() { return MainTextureLoader.get(); }
	ShaderLoader* GetShaderLoader() { return MainShaderLoader.get(); }
	RenderPipelineManager* GetRenderPipelineManager() { return MainRenderPipelineManager.get(); }
	AllocationHandler* GetAllocationHandler() { return MainAllocationHandler.get(); }
	RenderCoreResources* GetRenderCoreResources() { return &Resources; }

protected:

	void InitializeVulkan(WindowInstance* window)
	{
		Resources.MainWindow = window;
		Resources.vkWindowExtent.height = window->GetHeight();
		Resources.vkWindowExtent.width = window->GetWidth();
		InitVulkan();
		InitAllocationHandler();
		InitSwapchain();
		InitCommandPools();
		InitSyncStructures();
		InitSubSystems();
		InitDescriptors();
		MainRenderPipelineManager->InitEngineRenderPipelines();
		InitImgui();
		InitSamplers();
	}

	bool CheckIfRenderingConditionsMet()
	{
		if (Resources.MainWindow->WasResized() || Resources.bResizeNeeded)
		{
			ResizeSwapChain();
		}
		if (Resources.MainWindow->IsMinimized())
		{
			return false;
		}
		return true;
	}

	void WaitForGpuIdle()
	{
		vkDeviceWaitIdle(Resources.vkDevice);
	}

protected:

	void ResizeSwapChain()
	{
		vkDeviceWaitIdle(Resources.vkDevice);

		DestroySwapchain();

		Resources.vkWindowExtent.width = Resources.MainWindow->GetWidth();
		Resources.vkWindowExtent.height = Resources.MainWindow->GetHeight();

		CreateSwapchain(Resources.vkWindowExtent.width, Resources.vkWindowExtent.height);

		Resources.bResizeNeeded = false;
	}

	void RenderStart()
	{
		// Wait until the gpu has finished rendering the last frame. Timeout of 1 second
		VK_CHECK(vkWaitForFences(Resources.vkDevice, 1, &Resources.GetCurrentFrame().vkRenderFence, true, 1000000000));

		Resources.GetCurrentFrame().vkDeletionQueue.flush();
		Resources.GetCurrentFrame().vkFrameDescriptors.ClearPools(Resources.vkDevice);

		VK_CHECK(vkResetFences(Resources.vkDevice, 1, &Resources.GetCurrentFrame().vkRenderFence));

		// Request image from the swapchain
		VkResult e = vkAcquireNextImageKHR(Resources.vkDevice, Resources.vkSwapchain, 1000000000, Resources.GetCurrentFrame().vkSwapchainSemaphore, nullptr, &Resources.CurrentSwapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			Resources.bResizeNeeded = true;
			return;
		}
		else
		{
			VK_CHECK(e);
		}

		// Naming it cmd for shorter writing
		Resources.CurrentCMD = Resources.GetCurrentFrame().vkMainCommandBuffer;

		// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(Resources.CurrentCMD, 0));

		// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::CommandBuffer_BeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);


		// RECORDING TO COMMAND BUFFER
		// Start the command buffer recording
		VK_CHECK(vkBeginCommandBuffer(Resources.CurrentCMD, &cmdBeginInfo));

		Resources.vkDrawExtent.height = std::min(Resources.vkSwapchainExtent.height, Resources.vkDrawImage.imageExtent.height) * Resources.RenderScale;
		Resources.vkDrawExtent.width = std::min(Resources.vkSwapchainExtent.width, Resources.vkDrawImage.imageExtent.width) * Resources.RenderScale;


		// Transition our main draw image into general layout so we can write into it
		// We will overwrite it all so we don't care about what was the older layout
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkDrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		DrawBackground(Resources.CurrentCMD);

		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkDrawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkDepthImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		// Begin a render pass connected to our draw image
		VkRenderingAttachmentInfo colorAttachment = vkInit::AttachmentInfo(Resources.vkDrawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = vkInit::Depth_AttachmentInfo(Resources.vkDepthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkInit::RenderingInfo(Resources.vkWindowExtent, &colorAttachment, &depthAttachment);

		vkCmdBeginRendering(Resources.CurrentCMD, &renderInfo);
		RenderImgui();
	}

	void EndRecording()
	{
		vkCmdEndRendering(Resources.CurrentCMD);

		// Transition the draw image and the swapchain image into their correct transfer layouts
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkDrawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkSwapchainImages[Resources.CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Execute a copy from the draw image into the swapchain
		vkUtil::CopyImageToImage(Resources.CurrentCMD, Resources.vkDrawImage.image, Resources.vkSwapchainImages[Resources.CurrentSwapchainImageIndex], Resources.vkDrawExtent, Resources.vkSwapchainExtent);

		// Set swapchain image layout to Attachment Optimal so we can draw it
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkSwapchainImages[Resources.CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Draw imgui into the swapchain image
		DrawImgui(Resources.CurrentCMD, Resources.vkSwapchainImageViews[Resources.CurrentSwapchainImageIndex]);

		// Set swapchain image layout to Present so we can draw it
		vkUtil::TransitionImage(Resources.CurrentCMD, Resources.vkSwapchainImages[Resources.CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(Resources.CurrentCMD));

		// COMMAND BUFFER RECORDING ENDED
	}

	void SubmitToQueue()
	{
		// Prepare the submission to the queue. 
		// We want to wait on the vkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
		// We will signal the vkRenderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkInit::CommandBuffer_SubmitInfo(Resources.CurrentCMD);

		VkSemaphoreSubmitInfo waitInfo = vkInit::Semaphore_SubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, Resources.GetCurrentFrame().vkSwapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkInit::Semaphore_SubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, Resources.GetCurrentFrame().vkRenderSemaphore);

		VkSubmitInfo2 submit = vkInit::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

		// Submit command buffer to the queue and execute it.
		// vkRenderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(Resources.vkGraphicsQueue, 1, &submit, Resources.GetCurrentFrame().vkRenderFence));

		// Prepare present
		// This will put the image we just rendered to into the visible window.
		// We want to wait on the vkRenderSemaphore for that, 
		// As its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &Resources.vkSwapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &Resources.GetCurrentFrame().vkRenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &Resources.CurrentSwapchainImageIndex;

		VkResult e = vkQueuePresentKHR(Resources.vkGraphicsQueue, &presentInfo);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			Resources.bResizeNeeded = true;
			return;
		}
		else
		{
			VK_CHECK(e);
		}
		// Increase the number of frames drawn
		Resources.RenderFrameNumber++;
	}

	void DrawBackground(VkCommandBuffer cmd)
	{
		// Clear sceen
		// VkClearColorValue clearValue;
		// ClearValue = { { 0.0f, 0.0f, 0.0f, 1.0f } };
		// VkImageSubresourceRange clearRange = vkInit::Image_SubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
		// vkCmdClearColorImage(cmd, vkDrawImage.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
		
		// Bind the gradient drawing compute pipeline
		RenderPipeline gradientPipeline = MainRenderPipelineManager->GetEngineRenderPipeline(EngineRenderPipelines::GradientCompute);

		// Bind the background compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline.Pipeline);

		// Bind the descriptor set containing the draw image for the compute pipeline
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipeline.Layout, 0, 1, &Resources.vkDrawImageDescriptors, 0, nullptr);

		Resources.BackgroundGradientData.data3.x = Resources.vkWindowExtent.width;
		Resources.BackgroundGradientData.data3.y = Resources.vkWindowExtent.height;

		vkCmdPushConstants(cmd, gradientPipeline.Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ExtraPushConstants), &Resources.BackgroundGradientData);
		// Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, std::ceil(Resources.vkDrawExtent.width / 16.0), std::ceil(Resources.vkDrawExtent.height / 16.0), 1);
	}

	void DrawImgui(VkCommandBuffer cmd, VkImageView targetImageView)
	{
		VkRenderingAttachmentInfo colorAttachment = vkInit::AttachmentInfo(targetImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL);
		VkRenderingInfo renderInfo = vkInit::RenderingInfo(Resources.vkSwapchainExtent, &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
		vkCmdEndRendering(cmd);
	}

	void RenderImgui()
	{
		// Imgui new frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		if (ImGui::Begin("background")) {

			ImGui::SliderFloat("Render Scale", &Resources.RenderScale, 0.3f, 1.f);


			ImGui::Text("Gradient settings");

			ImGui::InputFloat4("Top", (float*)&Resources.BackgroundGradientData.data1);
			ImGui::InputFloat4("Bottom", (float*)&Resources.BackgroundGradientData.data2);
			//ImGui::InputFloat4("data3", (float*)&selected.data.data3);
			//ImGui::InputFloat4("data4", (float*)&selected.data.data4);
		}

		// Imgui UI to test
		//ImGui::ShowDemoWindow();
		ImGui::End();
		// Make imgui calculate internal draw structures
		ImGui::Render();
	}

private:

	void InitVulkan()
	{
		vkb::InstanceBuilder builder;

		// Make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("VulkanCore")
			.request_validation_layers(bUseValidationLayers)
			.use_default_debug_messenger()
			.require_api_version(1, 3, 0)
			.build();

		vkb::Instance vkb_inst = inst_ret.value();

		// Grab the instance 
		Resources.vkInstance = vkb_inst.instance;
		Resources.vkDebugMessenger = vkb_inst.debug_messenger;

		SDL_Vulkan_CreateSurface(Resources.MainWindow->GetWindow(), Resources.vkInstance, nullptr, &Resources.vkSurface);

		// Vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{};
		features.dynamicRendering = true;
		features.synchronization2 = true;
		features.maintenance4 = true;

		// Vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{};
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;

		// Use vk-bootstrap to select a gpu. 
		// We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(Resources.vkSurface)
			.select()
			.value();


		// Create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		Resources.vkDevice = vkbDevice.device;
		Resources.vkChosenGPU = physicalDevice.physical_device;

		// Use vk-bootstrap to get a Graphics queue
		Resources.vkGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		Resources.vkGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
	}

	void InitAllocationHandler()
	{
		MainAllocationHandler = std::make_unique<AllocationHandler>();
		MainAllocationHandler->InitAllocator(Resources.vkInstance, Resources.vkChosenGPU, Resources.vkDevice, Resources.vkGraphicsQueue, Resources.vkGraphicsQueueFamily);
	}

	void InitSubSystems()
	{
		MainMeshLoader = std::make_unique<MeshLoader>(MainAllocationHandler.get());
		MainTextureLoader = std::make_unique<TextureLoader>(MainAllocationHandler.get());
		MainShaderLoader = std::make_unique<ShaderLoader>(MainAllocationHandler.get(), &Resources);
		MainRenderPipelineManager = std::make_unique<RenderPipelineManager>(MainShaderLoader.get(), &Resources);
	}

	void InitSwapchain()
	{
		CreateSwapchain(Resources.vkWindowExtent.width, Resources.vkWindowExtent.height);

		// Draw image size will match the window
		VkExtent3D drawImageExtent = 
		{
			Resources.RenderResolutionWidth,
			Resources.RenderResolutionHeight,
			1
		};

		// Hardcoding the draw format to 64 bits per pixel. 16 bit floats for all 4 channels
		Resources.vkDrawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		Resources.vkDrawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = vkInit::Image_CreateInfo(Resources.vkDrawImage.imageFormat, drawImageUsages, drawImageExtent);

		// For the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Allocate and create the image
		vmaCreateImage(MainAllocationHandler->vmaAllocator, &rimg_info, &rimg_allocinfo, &Resources.vkDrawImage.image, &Resources.vkDrawImage.allocation, nullptr);

		// Build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = vkInit::Imageview_CreateInfo(Resources.vkDrawImage.imageFormat, Resources.vkDrawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(Resources.vkDevice, &rview_info, nullptr, &Resources.vkDrawImage.imageView));

		// Depth image
		Resources.vkDepthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
		Resources.vkDepthImage.imageExtent = drawImageExtent;
		VkImageUsageFlags depthImageUsages{};
		depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkImageCreateInfo dimg_info = vkInit::Image_CreateInfo(Resources.vkDepthImage.imageFormat, depthImageUsages, drawImageExtent);

		// Allocate and create the image
		vmaCreateImage(MainAllocationHandler->vmaAllocator, &dimg_info, &rimg_allocinfo, &Resources.vkDepthImage.image, &Resources.vkDepthImage.allocation, nullptr);

		// Build a image-view for the depth image to use for rendering
		VkImageViewCreateInfo dview_info = vkInit::Imageview_CreateInfo(Resources.vkDepthImage.imageFormat, Resources.vkDepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

		VK_CHECK(vkCreateImageView(Resources.vkDevice, &dview_info, nullptr, &Resources.vkDepthImage.imageView));

		// Add to deletion queues
		MainDeletionQueue.push_back([=]() {
			vkDestroyImageView(Resources.vkDevice, Resources.vkDrawImage.imageView, nullptr);
			vmaDestroyImage(MainAllocationHandler->vmaAllocator, Resources.vkDrawImage.image, Resources.vkDrawImage.allocation);

			vkDestroyImageView(Resources.vkDevice, Resources.vkDepthImage.imageView, nullptr);
			vmaDestroyImage(MainAllocationHandler->vmaAllocator, Resources.vkDepthImage.image, Resources.vkDepthImage.allocation);
			});
	}

	void CreateSwapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ Resources.vkChosenGPU, Resources.vkDevice, Resources.vkSurface };

		Resources.vkSwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = Resources.vkSwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			// Use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		Resources.vkSwapchainExtent = vkbSwapchain.extent;
		// Store swapchain and its related images
		Resources.vkSwapchain = vkbSwapchain.swapchain;
		Resources.vkSwapchainImages = vkbSwapchain.get_images().value();
		Resources.vkSwapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	void DestroySwapchain()
	{
		vkDestroySwapchainKHR(Resources.vkDevice, Resources.vkSwapchain, nullptr);

		// Destroy swapchain resources
		for (int i = 0; i < Resources.vkSwapchainImageViews.size(); i++) {
			vkDestroyImageView(Resources.vkDevice, Resources.vkSwapchainImageViews[i], nullptr);
		}
	}

	void InitCommandPools()
	{
		// Create a command pool for commands submitted to the graphics queue.
		// We also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPool_CreateInfo(Resources.vkGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			VK_CHECK(vkCreateCommandPool(Resources.vkDevice, &commandPoolInfo, nullptr, &Resources.vkFrames[i].vkCommandPool));

			// Allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::CommandBuffer_AllocateInfo(Resources.vkFrames[i].vkCommandPool, 1);
			
			VK_CHECK(vkAllocateCommandBuffers(Resources.vkDevice, &cmdAllocInfo, &Resources.vkFrames[i].vkMainCommandBuffer));
		}

		MainAllocationHandler->InitImmediateCommandPool();
	}

	void InitSyncStructures()
	{
		// Create syncronization structures
		// One fence to control when the gpu has finished rendering the frame,
		// And 2 semaphores to synchronize rendering with swapchain
		// We want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = vkInit::Fence_CreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkInit::Semaphore_CreateInfo();

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(Resources.vkDevice, &fenceCreateInfo, nullptr, &Resources.vkFrames[i].vkRenderFence));
			VK_CHECK(vkCreateSemaphore(Resources.vkDevice, &semaphoreCreateInfo, nullptr, &Resources.vkFrames[i].vkSwapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(Resources.vkDevice, &semaphoreCreateInfo, nullptr, &Resources.vkFrames[i].vkRenderSemaphore));
		}
		MainAllocationHandler->InitSyncStructures();
	}

	void InitDescriptors()
	{
		for (int i = 0; i < FRAME_OVERLAP; i++) {
			// Create a descriptor pool
			std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
			};

			Resources.vkFrames[i].vkFrameDescriptors = DescriptorAllocatorGrowable{};
			Resources.vkFrames[i].vkFrameDescriptors.Init(Resources.vkDevice, 1000, frame_sizes);

			MainDeletionQueue.push_back([&, i]() {
				Resources.vkFrames[i].vkFrameDescriptors.DestroyPools(Resources.vkDevice);
				});
		}
		
		// Create a descriptor pool that will hold 5 sets with 1 image each
		std::vector<DescriptorAllocator::PoolSizeRatio> sizes =
		{
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
		};

		DrawDescriptorAllocator.InitPool(Resources.vkDevice, 10, sizes);
		
		// Make the descriptor set layout for draw image
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		Resources.vkDrawImageDescriptorLayout = builder.Build(Resources.vkDevice, VK_SHADER_STAGE_COMPUTE_BIT);
		MainDeletionQueue.push_back([&]() 
			{
				vkDestroyDescriptorSetLayout(Resources.vkDevice, Resources.vkDrawImageDescriptorLayout, nullptr);
			}
		);

		// Allocate a descriptor set for our draw image
		Resources.vkDrawImageDescriptors = DrawDescriptorAllocator.Allocate(Resources.vkDevice, Resources.vkDrawImageDescriptorLayout);

		DescriptorWriter writer;

		writer.WriteImage(0, Resources.vkDrawImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		writer.UpdateSet(Resources.vkDevice, Resources.vkDrawImageDescriptors);

	}

	void InitImgui()
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
		VK_CHECK(vkCreateDescriptorPool(Resources.vkDevice, &pool_info, nullptr, &imguiPool));

		// 2: initialize imgui library
		// This initializes the core structures of imgui
		ImGui::CreateContext();

		// This initializes imgui for SDL
		ImGui_ImplSDL3_InitForVulkan(Resources.MainWindow->GetWindow());


		/*
		typedef struct VkPipelineRenderingCreateInfo {
    VkStructureType    sType;
    const void*        pNext;
    uint32_t           viewMask;
    uint32_t           colorAttachmentCount;
    const VkFormat*    pColorAttachmentFormats;
    VkFormat           depthAttachmentFormat;
    VkFormat           stencilAttachmentFormat;
} VkPipelineRenderingCreateInfo;
		
		*/
		VkPipelineRenderingCreateInfo dynamic_rendering_info = {};
		dynamic_rendering_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		dynamic_rendering_info.pNext = 0;
		dynamic_rendering_info.viewMask = 0;
		dynamic_rendering_info.colorAttachmentCount = 1;
		dynamic_rendering_info.pColorAttachmentFormats = &Resources.vkSwapchainImageFormat;
		dynamic_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
		dynamic_rendering_info.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		
		// This initializes imgui for Vulkan
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = Resources.vkInstance;
		init_info.PhysicalDevice = Resources.vkChosenGPU;
		init_info.Device = Resources.vkDevice;
		init_info.Queue = Resources.vkGraphicsQueue;
		init_info.DescriptorPool = imguiPool;
		init_info.MinImageCount = 3;
		init_info.ImageCount = 3;
		init_info.UseDynamicRendering = true;
		init_info.PipelineRenderingCreateInfo = dynamic_rendering_info;

		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		ImGui_ImplVulkan_Init(&init_info);
		ImGui_ImplVulkan_CreateFontsTexture();

		// Add the destroy the imgui created structures
		MainDeletionQueue.push_back([=]() {
			ImGui_ImplVulkan_DestroyFontsTexture();
			vkDestroyDescriptorPool(Resources.vkDevice, imguiPool, nullptr);
			ImGui_ImplVulkan_Shutdown();
			});

		Resources.MainWindow->SubscribeToEvents([&](SDL_Event e) {
			ImGui_ImplSDL3_ProcessEvent(&e);
			});
	}
	
	void InitSamplers()
	{
		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

		sampl.magFilter = VK_FILTER_NEAREST;
		sampl.minFilter = VK_FILTER_NEAREST;

		vkCreateSampler(Resources.vkDevice, &sampl, nullptr, &Resources.DefaultSamplerNearest);

		sampl.magFilter = VK_FILTER_LINEAR;
		sampl.minFilter = VK_FILTER_LINEAR;
		vkCreateSampler(Resources.vkDevice, &sampl, nullptr, &Resources.DefaultSamplerLinear);
		
		MainDeletionQueue.push_back([&]() {
			vkDestroySampler(Resources.vkDevice, Resources.DefaultSamplerNearest, nullptr);
			vkDestroySampler(Resources.vkDevice, Resources.DefaultSamplerLinear, nullptr);
			});
	}

	void Cleanup()
	{
		// Make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(Resources.vkDevice);
		MainDeletionQueue.flush();

		MainRenderPipelineManager->CleanEngineRenderPipelines();
		MainShaderLoader->DestroyShaders();

		DrawDescriptorAllocator.DestroyPool(Resources.vkDevice);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(Resources.vkDevice, Resources.vkFrames[i].vkCommandPool, nullptr);

			// Destroy sync objects
			vkDestroyFence(Resources.vkDevice, Resources.vkFrames[i].vkRenderFence, nullptr);
			vkDestroySemaphore(Resources.vkDevice, Resources.vkFrames[i].vkRenderSemaphore, nullptr);
			vkDestroySemaphore(Resources.vkDevice, Resources.vkFrames[i].vkSwapchainSemaphore, nullptr);
			Resources.vkFrames[i].vkDeletionQueue.flush();
		}

		MainAllocationHandler->Cleanup();

		DestroySwapchain();

		vkDestroySurfaceKHR(Resources.vkInstance, Resources.vkSurface, nullptr);
		vkDestroyDevice(Resources.vkDevice, nullptr);

		vkb::destroy_debug_utils_messenger(Resources.vkInstance, Resources.vkDebugMessenger);
		vkDestroyInstance(Resources.vkInstance, nullptr);

		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
		
	}

	// Main Subsystems
	std::unique_ptr<AllocationHandler> MainAllocationHandler;
	std::unique_ptr<MeshLoader> MainMeshLoader;
	std::unique_ptr<TextureLoader> MainTextureLoader;
	std::unique_ptr<ShaderLoader> MainShaderLoader;
	std::unique_ptr<RenderPipelineManager> MainRenderPipelineManager;

	bool bUseValidationLayers = true;

	// Class specific
	DeletionQueue MainDeletionQueue;
	DescriptorAllocator DrawDescriptorAllocator;

	// Sharable resources
	RenderCoreResources Resources;
};

