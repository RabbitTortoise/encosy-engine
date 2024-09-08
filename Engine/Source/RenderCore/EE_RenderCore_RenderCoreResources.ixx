module;
#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <VkBootstrapDispatch.h>

export module EE_RenderCore_Resources;

export import EE_Core_WindowManager;
export import EE_RenderCore_VulkanTypes;
export import EE_RenderCore_VulkanUtilities;
export import EE_RenderCore_VulkanDescriptors;


export import <vector>;
export import <array>;
export import <mutex>;



export constexpr unsigned int FRAME_OVERLAP = 2;


export struct FrameData
{
	//bool RaytracingEnabled = true;

	VkSemaphore vkSwapchainSemaphore, vkRenderSemaphore;
	VkFence vkRenderFence;

	VkCommandPool vkCommandPool;
	VkCommandBuffer vkMainCommandBuffer;

	DeletionQueue vkDeletionQueue;
	DescriptorAllocatorGrowable vkFrameDescriptors;

	VkAccelerationStructureKHR TopLevelAccelerationStructure;

	VkQueue vkGraphicsQueue; // For graphics commands
	uint32_t CurrentSwapchainImageIndex;
};

export struct RenderCoreResources
{
	bool ValidationLayersEnabled = false;
	bool GPUValidationLayersEnabled = false;

	// Main handles
	VkInstance vkInstance; // Vulkan library handle
	VkDebugUtilsMessengerEXT vkDebugMessenger; // Vulkan debug output handle
	VkPhysicalDevice vkChosenGPU; // GPU chosen as the default device
	VkDevice vkDevice; // Vulkan device for commands
	VkSurfaceKHR vkSurface; // Vulkan window surface
	VkQueue vkGraphicsSetupQueue; // For graphics commands
	VkQueue vkTransferQueue; // For transfer operations
	uint32_t vkGraphicsQueueFamily;
	uint32_t vkTransferQueueFamily;

	// Render window resources
	VkExtent2D vkWindowExtent = { 1920 , 1080 };
	//WindowInstance* MainWindow = nullptr;
	bool bResizeNeeded = false;
	uint32_t RenderResolutionWidth = 2560;
	uint32_t RenderResolutionHeight = 1440;
	float RenderScale = 1.f;

	// Frame swapchain handles

	VkSwapchainKHR vkSwapchain; //Swapchain used to render and present images
	VkFormat vkSwapchainImageFormat; //The image format swapchain uses
	std::vector<VkImage> vkSwapchainImages; // Handles to image objects
	std::vector<VkImageView> vkSwapchainImageViews; // Wrappers to specific images
	VkExtent2D vkSwapchainExtent; //
	FrameData vkFrames[FRAME_OVERLAP];
		
	unsigned int RenderFrameNumber = 2; // Trick to ensure GetPreviousFrame does not try to get negative frame.
	FrameData& GetCurrentFrame() { return vkFrames[RenderFrameNumber % FRAME_OVERLAP]; };
	FrameData& GetPreviousFrame() { return vkFrames[(RenderFrameNumber-1) % FRAME_OVERLAP]; };

	// Draw image resources
	AllocatedImage vkDrawImage;
	AllocatedImage vkDepthImage;
	VkExtent2D vkDrawExtent;
	VkDescriptorSet vkDrawImageDescriptors;
	VkDescriptorSetLayout vkDrawImageDescriptorLayout;

	// Gradient pipeline resources
	VkPipeline vkGradientPipeline;
	VkPipelineLayout vkGradientPipelineLayout;
	ExtraPushConstants BackgroundGradientData =
	{
		glm::vec4(0.4f, 0.6f, 0.9f, 1),
		glm::vec4(0.088f, 0.133f, 0.2f, 1),
		glm::vec4(0.0f, 0.0f, 0.0f, 1),
		glm::vec4(0.0f, 0.0f, 0.0f, 1)
	};

	// Texture sampler resources
	VkSampler DefaultSamplerLinear;
	VkSampler DefaultSamplerNearest;

	// Global lighting data
	LitLightingData GlobalLightingData =
	{
		.ambientLightColor = glm::vec3(1.0f, 1.0f, 1.0f),
		.ambientLightStrength = 0.0f,
		.directionalLightDir = glm::vec3(-0.5f, -0.75f, 1.0f),
		.directionalLightStrength = 1.0f,
		.directionalLightColor = glm::vec3(1.0f, 1.0f, 1.0f),
	};

	// Extensions
	vkb::DispatchTable DispatchTable;
};