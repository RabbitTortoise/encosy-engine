module;
#include <vulkan/vulkan.h>
#include <glm/vec4.hpp>

export module RenderCore.Resources;

export import EncosyEngine.WindowManager;
export import RenderCore.VulkanTypes;
export import <vector>;

export constexpr unsigned int FRAME_OVERLAP = 2;

export struct RenderCoreResources
{

	// Main handles
	VkInstance vkInstance; // Vulkan library handle
	VkDebugUtilsMessengerEXT vkDebugMessenger; // Vulkan debug output handle
	VkPhysicalDevice vkChosenGPU; // GPU chosen as the default device
	VkDevice vkDevice; // Vulkan device for commands
	VkSurfaceKHR vkSurface; // Vulkan window surface
	VkQueue vkGraphicsQueue; // 
	uint32_t vkGraphicsQueueFamily; //

	// Render window resources
	VkExtent2D vkWindowExtent = { 1920 , 1080 };
	WindowInstance* MainWindow = nullptr;
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
		
	unsigned int RenderFrameNumber = 0;
	FrameData& GetCurrentFrame() { return vkFrames[RenderFrameNumber % FRAME_OVERLAP]; };

	// Draw image resources
	AllocatedImage vkDrawImage;
	AllocatedImage vkDepthImage;
	VkExtent2D vkDrawExtent;
	VkDescriptorSet vkDrawImageDescriptors;
	VkDescriptorSetLayout vkDrawImageDescriptorLayout;

	// Ongoing render handles
	VkCommandBuffer CurrentCMD;
	uint32_t CurrentSwapchainImageIndex;

	// Gradient pipeline resources
	VkPipeline vkGradientPipeline;
	VkPipelineLayout vkGradientPipelineLayout;
	ExtraPushConstants BackgroundGradientData =
	{
		glm::vec4(0.2f, 0.2f, 0.6f, 1),
		glm::vec4(0.025f, 0.0f, 0.025f, 1),
		glm::vec4(0.0f, 0.0f, 0.0f, 1),
		glm::vec4(0.0f, 0.0f, 0.0f, 1)
	};

	// Texture sampler resources
	VkSampler DefaultSamplerLinear;
	VkSampler DefaultSamplerNearest;

};