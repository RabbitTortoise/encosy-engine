module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

export module RenderCore.AllocationHandler;

import RenderCore.VulkanInitializers;
import RenderCore.VulkanErrorLogger;
import RenderCore.VulkanUtilities;
import RenderCore.VulkanDescriptors;
import RenderCore.VulkanTypes;

import <string>;
import <vector>;
import <span>;
import <functional>;


export class AllocationHandler
{
	friend class RenderCore;

public:
	AllocationHandler() {}
	~AllocationHandler() { Cleanup(); }

	void Cleanup()
	{
		HandlerDeletionQueue.flush();
	}

	void InitAllocator(VkInstance instance, VkPhysicalDevice gpu, VkDevice device, VkQueue queue, uint32_t queueFamily)
	{
		vkInstance = instance;
		vkChosenGPU = gpu;
		vkDevice = device;
		vkGraphicsQueue = queue;
		vkGraphicsQueueFamily = queueFamily;

		// Initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = vkChosenGPU;
		allocatorInfo.device = vkDevice;
		allocatorInfo.instance = vkInstance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

		HandlerDeletionQueue.push_back([&]() {
			vmaDestroyAllocator(vmaAllocator);
			});
	}

	void InitImmediateCommandPool()
	{
		VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPool_CreateInfo(vkGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		
		VK_CHECK(vkCreateCommandPool(vkDevice, &commandPoolInfo, nullptr, &vkImmediateCommandPool));

		// Allocate the command buffer for immediate submits
		VkCommandBufferAllocateInfo cmdAllocInfo = vkInit::CommandBuffer_AllocateInfo(vkImmediateCommandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(vkDevice, &cmdAllocInfo, &vkImmediateCommandBuffer));

		HandlerDeletionQueue.push_back([=]() {
			vkDestroyCommandPool(vkDevice, vkImmediateCommandPool, nullptr);
			});
	}

	void InitSyncStructures()
	{
		VkFenceCreateInfo fenceCreateInfo = vkInit::Fence_CreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VK_CHECK(vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &vkImmediateFence));
		HandlerDeletionQueue.push_back([=]() { vkDestroyFence(vkDevice, vkImmediateFence, nullptr); });
	}

	void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function)
	{
		VK_CHECK(vkResetFences(vkDevice, 1, &vkImmediateFence));
		VK_CHECK(vkResetCommandBuffer(vkImmediateCommandBuffer, 0));

		VkCommandBuffer cmd = vkImmediateCommandBuffer;

		VkCommandBufferBeginInfo cmdBeginInfo = vkInit::CommandBuffer_BeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		function(cmd);

		VK_CHECK(vkEndCommandBuffer(cmd));

		VkCommandBufferSubmitInfo cmdinfo = vkInit::CommandBuffer_SubmitInfo(cmd);
		VkSubmitInfo2 submit = vkInit::SubmitInfo(&cmdinfo, nullptr, nullptr);

		// Submit command buffer to the queue and execute it.
		// vkImmediateFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(vkGraphicsQueue, 1, &submit, vkImmediateFence));

		VK_CHECK(vkWaitForFences(vkDevice, 1, &vkImmediateFence, true, 9999999999));
	}

	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	AllocatedBuffer CreateGPUBuffer(size_t allocSize, VkBufferUsageFlags usage)
	{
		// Allocate buffer
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		AllocatedBuffer newBuffer;

		// Allocate the buffer
		VK_CHECK(vmaCreateBuffer(vmaAllocator,
			&bufferInfo,
			&vmaallocInfo,
			&newBuffer.buffer,
			&newBuffer.allocation,
			&newBuffer.info));

		return newBuffer;
	}

	AllocatedBuffer CreateBuffer(size_t allocSize, VkBufferUsageFlags usage)
	{
		// Allocate buffer
		VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.pNext = nullptr;
		bufferInfo.size = allocSize;

		bufferInfo.usage = usage;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		AllocatedBuffer newBuffer;

		// Allocate the buffer
		VK_CHECK(vmaCreateBuffer(vmaAllocator,
			&bufferInfo,
			&vmaallocInfo,
			&newBuffer.buffer,
			&newBuffer.allocation,
			&newBuffer.info));

		return newBuffer;
	}

	void DestroyBuffer(const AllocatedBuffer& buffer)
	{
		vmaDestroyBuffer(vmaAllocator, buffer.buffer, buffer.allocation);
	}

	template <class T>
	void WriteToBuffer(AllocatedBuffer& buffer, T* dataToWrite)
	{
		T* data = (T*)buffer.allocation->GetMappedData();
		*data = *dataToWrite;
	}

	GPUModelMatrixBuffer CreateModelMatrixBuffer(const std::vector<glm::mat4>& matrixes)
	{
		const size_t bufferSize = matrixes.size() * sizeof(glm::mat4);
		GPUModelMatrixBuffer newBuffer;

		// Create buffer
		newBuffer.matrixBuffer = CreateGPUBuffer(bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newBuffer.matrixBuffer.buffer };
		newBuffer.matrixBufferAddress = vkGetBufferDeviceAddress(vkDevice, &deviceAdressInfo);

		void* matrixData = newBuffer.matrixBuffer.allocation->GetMappedData();

		// Copy vertex buffer
		memcpy(matrixData, matrixes.data(), bufferSize);

		return newBuffer;
	}


	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	GPUMeshBuffers UploadMeshToGPU(std::span<Vertex> vertices, std::span<uint32_t> indices)
	{

		const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		GPUMeshBuffers newSurface;

		// Create vertex buffer
		newSurface.vertexBuffer = CreateGPUBuffer(vertexBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo deviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
		newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(vkDevice, &deviceAdressInfo);

		// Create index buffer
		newSurface.indexBuffer = CreateGPUBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

		void* vertexData = newSurface.vertexBuffer.allocation->GetMappedData();
		void* indexData = newSurface.indexBuffer.allocation->GetMappedData();

		// Copy vertex buffer
		memcpy(vertexData, vertices.data(), vertexBufferSize);
		// Copy index buffer
		memcpy(indexData , indices.data(), indexBufferSize);

		HandlerDeletionQueue.push_back([=]() {
			DestroyBuffer(newSurface.indexBuffer);
			DestroyBuffer(newSurface.vertexBuffer);
			});

		return newSurface;
	}

	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	AllocatedImage AllocateAndUploadImageToGPU(void* data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped = false)
	{
		AllocatedImage newImage = AllocateImage(size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);
		UploadImageToGPU(data, size, newImage);

		return newImage;
	}


	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	AllocatedImage AllocateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped)
	{
		AllocatedImage newImage;
		newImage.imageFormat = format;
		newImage.imageExtent = size;

		VkImageCreateInfo img_info = vkInit::Image_CreateInfo(format, usage, size);
		if (mipmapped) {
			img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
		}

		// Always allocate images on dedicated GPU memory
		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		// Allocate and create the image
		VK_CHECK(vmaCreateImage(vmaAllocator, &img_info, &vmaallocInfo, &newImage.image, &newImage.allocation, nullptr));

		// If the format is a depth format, we will need to have it use the correct aspect flag
		VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
		if (format == VK_FORMAT_D32_SFLOAT) {
			aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
		}

		// Build a image-view for the image
		VkImageViewCreateInfo view_info = vkInit::Imageview_CreateInfo(format, newImage.image, aspectFlag);
		view_info.subresourceRange.levelCount = img_info.mipLevels;

		VK_CHECK(vkCreateImageView(vkDevice, &view_info, nullptr, &newImage.imageView));


		HandlerDeletionQueue.push_back([=]() {
			vkDestroyImageView(vkDevice, newImage.imageView, nullptr);
			vmaDestroyImage(vmaAllocator, newImage.image, newImage.allocation);
			});
		
		return newImage;
	}


	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	void UploadImageToGPU(void* data, VkExtent3D size, AllocatedImage newImage)
	{

		// Hardcoded to VK_FORMAT_R8G8B8A8_UNORM
		size_t data_size = size.depth * size.width * size.height * 4;

		AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		memcpy(uploadbuffer.info.pMappedData, data, data_size);


		ImmediateSubmit([&](VkCommandBuffer cmd) {
			TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			VkBufferImageCopy copyRegion = {};
			copyRegion.bufferOffset = 0;
			copyRegion.bufferRowLength = 0;
			copyRegion.bufferImageHeight = 0;

			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = size;

			// copy the buffer into the image
			vkCmdCopyBufferToImage(cmd, uploadbuffer.buffer, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			});

		DestroyBuffer(uploadbuffer);
	}

	void DestroyImage(const AllocatedImage& img)
	{
		vkDestroyImageView(vkDevice, img.imageView, nullptr);
		vmaDestroyImage(vmaAllocator, img.image, img.allocation);
	}

private:
	VmaAllocator vmaAllocator;

	// Needed vulkan handles
	VkInstance vkInstance;
	VkPhysicalDevice vkChosenGPU;
	VkDevice vkDevice;
	VkQueue vkGraphicsQueue;
	uint32_t vkGraphicsQueueFamily;

	// Immediate submit structures
	VkFence vkImmediateFence;
	VkCommandBuffer vkImmediateCommandBuffer;
	VkCommandPool vkImmediateCommandPool;

	// Allocated resources deletion queue
	DeletionQueue HandlerDeletionQueue;
};