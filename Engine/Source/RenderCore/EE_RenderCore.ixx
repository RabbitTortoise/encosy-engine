module;

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vma/vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <array>

export module EE_RenderCore;

import EE_Core_MatrixCalculations;
import EE_Core_WindowManager;
import EE_Core_WM_WindowInstance;
import EE_Encosy_EncosyWorld;

import EE_RenderCore_Resources;
import EE_RenderCore_RenderThread;
import EE_RenderCore_RaytracingResources;
import EE_RenderCore_VulkanInitializers;
import EE_RenderCore_VulkanErrorLogger;

import EE_RenderCore_VulkanUtilities;
import EE_RenderCore_VulkanDescriptors;
import EE_RenderCore_VulkanRaytracing;
import EE_RenderCore_AllocationHandler;
import EE_RenderCore_MeshLoader;
import EE_RenderCore_TextureLoader;
import EE_RenderCore_ShaderLoader;
import EE_RenderCore_RenderPipelineManager;
import EE_RenderCore_VulkanImgui;


import <memory>;
import <optional>;
import <string>;
import <span>;
import <functional>;
import <deque>;
import <filesystem>;
import <iostream>;
import <format>;
import <algorithm>;
import <numeric>;
import <mutex>;
import <barrier>;


export class RenderCore
{
	friend class EngineCore;

public:

	RenderCore(){}
	~RenderCore(){}

	MeshLoader* GetMeshLoader() { return MainMeshLoader.get(); }
	TextureLoader* GetTextureLoader() { return MainTextureLoader.get(); }
	ShaderLoader* GetShaderLoader() { return MainShaderLoader.get(); }
	RenderPipelineManager* GetRenderPipelineManager() { return MainRenderPipelineManager.get(); }
	AllocationHandler* GetAllocationHandler() { return MainAllocationHandler.get(); }
	RenderCoreResources* GetRenderCoreResources() { return &Resources; }
	RaytracingResources* GetRaytracingResources(){ return &RtResources; }

	TextureSetID RegisterTextureSetForRaytracingUsage(PBRTextureSet set)
	{
		return MainVulkanRaytracing->RegisterTextureSetForRaytracingUsage(set);
	}

	void RegisterMeshForRaytracingUsage(MeshID meshId)
	{
		MainVulkanRaytracing->RegisterMeshForRaytracingUsage(meshId);
	}


	void InitializeVulkan(EncosyWorld* world)
	{
		MainWorld = world;
		Resources.vkWindowExtent.height = EncosyEngine::WindowManager::GetMainWindowHeight();
		Resources.vkWindowExtent.width = EncosyEngine::WindowManager::GetMainWindowWidth();

		StartBarrier = new std::barrier(2);
		FinishBarrier = new std::barrier(2);
		MainRenderThread.CreateRenderThread(StopSource.get_token(), StartBarrier, FinishBarrier);
		auto token = StartBarrier->arrive();

		InitVulkan();
		InitAllocationHandler();
		CreateSwapchain();
		CreateDrawImages();
		InitCommandPools();
		InitSyncStructures();
		InitSubSystems();
		InitDescriptors();
		MainVulkanRaytracing->InitRayTracing();
		MainRenderPipelineManager->InitEngineRenderPipelines();
		InitImgui();
		InitSamplers();
	}



	void InitVulkan()
	{
		vkb::InstanceBuilder builder;

		// GPU validation features
		VkValidationFeatureEnableEXT enable_features1 = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT };
		VkValidationFeatureEnableEXT enable_features2 = { VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT };

		// Make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("VulkanCore");
		inst_ret = inst_ret.request_validation_layers(Resources.ValidationLayersEnabled);
		inst_ret = inst_ret.use_default_debug_messenger();
		inst_ret = inst_ret.require_api_version(1, 3, 0);
		inst_ret = inst_ret.desire_api_version(1, 3, 0);
		if (Resources.GPUValidationLayersEnabled)
		{
			inst_ret = inst_ret.add_validation_feature_enable(enable_features1);
			inst_ret = inst_ret.add_validation_feature_enable(enable_features2);
		}

		auto int_build = inst_ret.build();

		vkb::Instance vkb_inst = int_build.value();

		// Grab the instance 
		Resources.vkInstance = vkb_inst.instance;
		Resources.vkDebugMessenger = vkb_inst.debug_messenger;

		auto test = EncosyEngine::WindowManager::GetMainWindow();
		SDL_Vulkan_CreateSurface(EncosyEngine::WindowManager::GetMainWindow(), Resources.vkInstance, nullptr, &Resources.vkSurface);

		// Vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{};
		features.dynamicRendering = true;
		features.synchronization2 = true;
		features.maintenance4 = true;

		// Vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{};
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;
		features12.shaderSampledImageArrayNonUniformIndexing = true;
		features12.descriptorBindingSampledImageUpdateAfterBind = true;
		features12.descriptorBindingPartiallyBound = true;
		features12.descriptorBindingUpdateUnusedWhilePending = true;
		features12.descriptorBindingVariableDescriptorCount = true;
		features12.descriptorBindingStorageImageUpdateAfterBind = true;
		features12.descriptorBindingUniformBufferUpdateAfterBind = true;
		features12.descriptorBindingStorageBufferUpdateAfterBind = true;
		// Enables use of runtimeDescriptorArrays in SPIR-V shaders.
		features12.runtimeDescriptorArray = true; 

		// Raytracing features
		VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
		asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		asFeatures.pNext = nullptr;
		asFeatures.accelerationStructure = true;
		asFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = true;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtpFeatures{};
		rtpFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		rtpFeatures.pNext = nullptr;
		rtpFeatures.rayTracingPipeline = true;

		// Use vk-bootstrap to select a gpu. 
		// We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		
		selector = selector.set_minimum_version(1, 3);
		selector = selector.set_required_features_13(features);
		selector = selector.set_required_features_12(features12);
		selector = selector.set_surface(Resources.vkSurface);
		// Ray tracing related extensions
		selector = selector.add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		selector = selector.add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		// Required by VK_KHR_acceleration_structure
		selector = selector.add_required_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		selector = selector.add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		selector = selector.add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		// Required for VK_KHR_ray_tracing_pipeline
		selector = selector.add_required_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		// Required by VK_KHR_spirv_1_4
		selector = selector.add_required_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		selector = selector.add_required_extension_features(asFeatures);
		selector = selector.add_required_extension_features(rtpFeatures);
		selector = selector.required_device_memory_size(VkDeviceSize(4000000000));
			
		vkb::PhysicalDevice physicalDevice = selector.select().value();
		std::vector<VkQueueFamilyProperties> queueFamilyProperties = physicalDevice.get_queue_families();

		// Create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		// Queue setup
		std::vector<vkb::CustomQueueDescription> queue_descriptions;
		for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {

			bool graphicsBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == static_cast<uint32_t>(VK_QUEUE_GRAPHICS_BIT);
			bool computeBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == static_cast<uint32_t>(VK_QUEUE_COMPUTE_BIT);
			bool transferBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) == static_cast<uint32_t>(VK_QUEUE_TRANSFER_BIT);
			bool sparseBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == static_cast<uint32_t>(VK_QUEUE_SPARSE_BINDING_BIT);
			bool protectedBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_PROTECTED_BIT) == static_cast<uint32_t>(VK_QUEUE_PROTECTED_BIT);
			bool videoDecodeBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) == static_cast<uint32_t>(VK_QUEUE_VIDEO_DECODE_BIT_KHR);
			bool videoEncodeBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR) == static_cast<uint32_t>(VK_QUEUE_VIDEO_ENCODE_BIT_KHR);
			bool opticalFlowBit = static_cast<uint32_t>(queueFamilyProperties[i].queueFlags & VK_QUEUE_OPTICAL_FLOW_BIT_NV) == static_cast<uint32_t>(VK_QUEUE_OPTICAL_FLOW_BIT_NV);

			// Main Graphics Queue requirements
			if (graphicsBit && computeBit && transferBit)
			{
				if (queueFamilyProperties[i].queueCount < 3)
				{
					fmt::println("ERROR: GPU does not support 3 graphics queues!");
					abort();
				}
				queue_descriptions.emplace_back(i, std::vector<float>{ 1.0f , 1.0f, 0.5f});
			}
			// Transfer Queue requirements
			else if (
				transferBit &&
				!graphicsBit && !computeBit && !videoDecodeBit && !videoEncodeBit && !opticalFlowBit)
			{
				queue_descriptions.emplace_back(i, std::vector<float>{ 1.0f });
			}
		}
		deviceBuilder.custom_queue_setup(queue_descriptions);

		
		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		Resources.vkDevice = vkbDevice.device;
		Resources.vkChosenGPU = physicalDevice.physical_device;
		
		// Use vk-bootstrap to get a Graphics queue
		Resources.vkGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		vkGetDeviceQueue(Resources.vkDevice, Resources.vkGraphicsQueueFamily, 0, &Resources.vkFrames[0].vkGraphicsQueue);
		vkGetDeviceQueue(Resources.vkDevice, Resources.vkGraphicsQueueFamily, 0, &Resources.vkFrames[1].vkGraphicsQueue);
		vkGetDeviceQueue(Resources.vkDevice, Resources.vkGraphicsQueueFamily, 2, &Resources.vkGraphicsSetupQueue);

		// Use vk-bootstrap to get a Transfer queue
		Resources.vkTransferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();
		Resources.vkTransferQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

		Resources.DispatchTable = vkbDevice.make_table();
	}

	void InitAllocationHandler()
	{
		MainAllocationHandler = std::make_unique<AllocationHandler>(&Resources, &RtResources);
	}

	void InitSubSystems()
	{
		MainMeshLoader = std::make_unique<MeshLoader>(MainAllocationHandler.get());
		MainTextureLoader = std::make_unique<TextureLoader>(MainAllocationHandler.get());
		MainShaderLoader = std::make_unique<ShaderLoader>(MainAllocationHandler.get(), &Resources);
		MainRenderPipelineManager = std::make_unique<RenderPipelineManager>(MainShaderLoader.get(), &Resources, &RtResources);
		MainVulkanRaytracing = std::make_unique<VulkanRaytracing>(&Resources, &RtResources, MainAllocationHandler.get(), MainMeshLoader.get(), MainTextureLoader.get(), MainRenderPipelineManager.get());
	}

	void CreateDrawImages()
	{
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

	void CreateSwapchain()
	{
		uint32_t width = Resources.vkWindowExtent.width;
		uint32_t height = Resources.vkWindowExtent.height;

		vkb::SwapchainBuilder swapchainBuilder{ Resources.vkChosenGPU, Resources.vkDevice, Resources.vkSurface };

		Resources.vkSwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = Resources.vkSwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			// Use vsync present mode
			//.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			// Use immediate mode
			//.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
			// Present the most recently updated image.
			.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
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
		MainVulkanImgui = std::make_unique<VulkanImgui>(&Resources);
		MainVulkanImgui->InitImgui(RtResources.EmProperties.minImportedHostPointerAlignment);
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

	void BuildRaytracingStructures()
	{
		MainVulkanRaytracing->CreateTextureSets();
		MainVulkanRaytracing->CreateModelBufferInfos();
		MainVulkanRaytracing->RecalculateBottomLevelAccelerationStructures();
		MainVulkanRaytracing->CreateRtShaderBindingTable();
		MainVulkanRaytracing->CreateRaytracingStorageImage();
	}

	bool CheckIfRenderingConditionsMet()
	{
		if (EncosyEngine::WindowManager::WasMainWindowResized() || Resources.bResizeNeeded)
		{
			ResizeSwapChain();
		}
		if (EncosyEngine::WindowManager::IsMainWindowMinimized())
		{
			return false;
		}
		return true;
	}

	void StopRenderingForcefully()
	{
		if (MainRenderThread.GetIsWorking())
		{
			FinishBarrier->arrive_and_wait();
		}
		MainRenderThread.ClearQueue();
		vkDeviceWaitIdle(Resources.vkDevice);
	}

	void ResizeSwapChain()
	{
		vkDeviceWaitIdle(Resources.vkDevice);

		DestroySwapchain();

		Resources.vkWindowExtent.width = EncosyEngine::WindowManager::GetMainWindowWidth();
		Resources.vkWindowExtent.height = EncosyEngine::WindowManager::GetMainWindowHeight();

		CreateSwapchain();

		MainVulkanRaytracing->ResizeRaytracingStorageImage();

		Resources.bResizeNeeded = false;
	}

	void SetupCommandBuffer()
	{
		FrameData& currentFrame = Resources.GetCurrentFrame();

		currentFrame.vkDeletionQueue.flush();
		currentFrame.vkFrameDescriptors.ClearPools(Resources.vkDevice);

		VK_CHECK(vkResetFences(Resources.vkDevice, 1, &currentFrame.vkRenderFence));

		{
			std::scoped_lock lock(SwapchainMutex);
			// Request image from the swapchain
			VkResult e = vkAcquireNextImageKHR(Resources.vkDevice, Resources.vkSwapchain, 1000000000, currentFrame.vkSwapchainSemaphore, nullptr, &currentFrame.CurrentSwapchainImageIndex);
			if (e == VK_ERROR_OUT_OF_DATE_KHR) {
				Resources.bResizeNeeded = true;
				return;
			}
			VK_CHECK(e);
		}
		// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(currentFrame.vkMainCommandBuffer, 0));

		// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::CommandBuffer_BeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// Start the command buffer recording
		VK_CHECK(vkBeginCommandBuffer(currentFrame.vkMainCommandBuffer, &cmdBeginInfo));

		// RECORDING TO COMMAND BUFFER
	}

	void RenderStart()
	{
		// Draw background
		Resources.vkDrawExtent.height = std::min(Resources.vkSwapchainExtent.height, Resources.vkDrawImage.imageExtent.height) * Resources.RenderScale;
		Resources.vkDrawExtent.width = std::min(Resources.vkSwapchainExtent.width, Resources.vkDrawImage.imageExtent.width) * Resources.RenderScale;
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkDrawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
		DrawBackground(Resources.GetCurrentFrame().vkMainCommandBuffer);

		// Copy background to raytracing result image
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkDrawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, RtResources.RaytracedImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyImageToImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkDrawImage.image, RtResources.RaytracedImage.image, Resources.vkSwapchainExtent, RtResources.RaytracedImageExtend);

		// Transition raytracing result image to shader usage
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, RtResources.RaytracedImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
	}

	void RenderEnd()
	{
		// Transition the draw image and the swapchain image into their correct transfer layouts
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkSwapchainImages[Resources.GetCurrentFrame().CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, RtResources.RaytracedImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		
		// Execute a copy from the draw image into the swapchain
		auto extent = RtResources.RaytracedImageExtend;
		extent.width *= Resources.RenderScale;
		extent.height *= Resources.RenderScale;
		CopyImageToImage(Resources.GetCurrentFrame().vkMainCommandBuffer, RtResources.RaytracedImage.image, Resources.vkSwapchainImages[Resources.GetCurrentFrame().CurrentSwapchainImageIndex], extent, Resources.vkSwapchainExtent);
	
		// Transition swapchain image for imgui redering
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkSwapchainImages[Resources.GetCurrentFrame().CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		RenderImgui();

		// Set swapchain image layout to Present so we can draw it
		TransitionImage(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkSwapchainImages[Resources.GetCurrentFrame().CurrentSwapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		// Finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(Resources.GetCurrentFrame().vkMainCommandBuffer));

		// COMMAND BUFFER RECORDING ENDED
	}

	void RenderImgui()
	{
		// Update debug panel
		MainVulkanImgui->UpdateImgui(MainWorld->GetWorldEntityManager()->GetCurrentEntityCount());
		// Draw imgui into the swapchain image
		MainVulkanImgui->RenderImgui(Resources.GetCurrentFrame().vkMainCommandBuffer, Resources.vkSwapchainImageViews[Resources.GetCurrentFrame().CurrentSwapchainImageIndex]);
	}

	void SubmitToQueue()
	{
		FrameData& previousFrame = Resources.GetPreviousFrame();
		FrameData& currentFrame = Resources.GetCurrentFrame();

		// Wait until the gpu has finished rendering the previous frame. We do not want to render two frames at once.
		FinishBarrier->arrive_and_wait();

		MainRenderThread.AddTask(std::bind_front(&RenderCore::SubmitTask, this), currentFrame);
		StartBarrier->arrive_and_wait();
		
		// Increase the number of frames drawn
		Resources.RenderFrameNumber++;
	}

	void SubmitTask(FrameData& currentFrame)
	{
		// Prepare the submission to the queue. 
		// We want to wait on the vkPresentSemaphore, as that semaphore is signaled when the swapchain is ready
		// We will signal the vkRenderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkInit::CommandBuffer_SubmitInfo(currentFrame.vkMainCommandBuffer);

		VkSemaphoreSubmitInfo waitInfo = vkInit::Semaphore_SubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.vkSwapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkInit::Semaphore_SubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.vkRenderSemaphore);

		VkSubmitInfo2 submit = vkInit::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

		// Submit command buffer to the queue and execute it.
		// vkRenderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(currentFrame.vkGraphicsQueue, 1, &submit, currentFrame.vkRenderFence));

		// Prepare present
		// This will put the image we just rendered to into the visible window.
		// We want to wait on the vkRenderSemaphore for that, 
		// As its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &Resources.vkSwapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &currentFrame.vkRenderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &currentFrame.CurrentSwapchainImageIndex;

		{
			std::scoped_lock lock(SwapchainMutex);
			VkResult e = vkQueuePresentKHR(currentFrame.vkGraphicsQueue, &presentInfo);
			if (e == VK_ERROR_OUT_OF_DATE_KHR) {
				Resources.bResizeNeeded = true;
				return;
			}
			else
			{
				VK_CHECK(e);
			}
		}

		VK_CHECK(vkWaitForFences(Resources.vkDevice, 1, &currentFrame.vkRenderFence, true, 10000000000));

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

		Resources.BackgroundGradientData.data3.x = Resources.vkWindowExtent.width * Resources.RenderScale;
		Resources.BackgroundGradientData.data3.y = Resources.vkWindowExtent.height * Resources.RenderScale;

		vkCmdPushConstants(cmd, gradientPipeline.Layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ExtraPushConstants), &Resources.BackgroundGradientData);
		// Execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, std::ceil(Resources.vkDrawExtent.width / 16.0), std::ceil(Resources.vkDrawExtent.height / 16.0), 1);
	}



	void Cleanup()
	{
		StopSource.request_stop();
		auto copyToken = StartBarrier->arrive();
		auto finishToken = FinishBarrier->arrive();
		MainRenderThread.Data->Thread.join();

		// Make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(Resources.vkDevice);

		MainDeletionQueue.flush();

		MainVulkanImgui->Cleanup();

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
		MainVulkanRaytracing->DeleteRaytracingStorageImage();
		MainAllocationHandler->Cleanup();

		DestroySwapchain();

		vkDestroySurfaceKHR(Resources.vkInstance, Resources.vkSurface, nullptr);
		vkDestroyDevice(Resources.vkDevice, nullptr);

		vkb::destroy_debug_utils_messenger(Resources.vkInstance, Resources.vkDebugMessenger);
		vkDestroyInstance(Resources.vkInstance, nullptr);


		delete MainRenderThread.Data;
		delete StartBarrier;
		delete FinishBarrier;
	}

	private:

	// Main Subsystems
	std::unique_ptr<AllocationHandler> MainAllocationHandler;
	std::unique_ptr<MeshLoader> MainMeshLoader;
	std::unique_ptr<TextureLoader> MainTextureLoader;
	std::unique_ptr<ShaderLoader> MainShaderLoader;
	std::unique_ptr<RenderPipelineManager> MainRenderPipelineManager;
	std::unique_ptr<VulkanRaytracing> MainVulkanRaytracing;
	std::unique_ptr<VulkanImgui> MainVulkanImgui;


	// Class specific
	DeletionQueue MainDeletionQueue;
	DescriptorAllocator DrawDescriptorAllocator;
	EncosyWorld* MainWorld;

	// Sharable resources
	RenderCoreResources Resources;
	RaytracingResources RtResources;

	std::vector<SystemID> RenderSystems;
	std::vector<SystemID> RaytracingRenderSystems;

	RenderThread MainRenderThread;
	std::stop_source StopSource;
	std::barrier<>* StartBarrier;
	std::barrier<>* FinishBarrier;

	std::mutex SwapchainMutex;
};

