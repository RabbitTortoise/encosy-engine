module;
#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <VkBootstrapDispatch.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "fmt/chrono.h"

export module RenderCore.AllocationHandler;

import RenderCore.VulkanInitializers;
import RenderCore.VulkanErrorLogger;
import RenderCore.VulkanUtilities;
import RenderCore.VulkanDescriptors;
import RenderCore.VulkanTypes;
import RenderCore.Resources;
import RenderCore.RaytracingResources;

import <string>;
import <vector>;
import <span>;
import <functional>;



export class AllocationHandler
{
	friend class RenderCore;

public:
	AllocationHandler(RenderCoreResources* resources, RaytracingResources* rtResources)
	{
		CoreResources = resources;
		RtResources = rtResources;
		vkInstance = resources->vkInstance;
		vkChosenGPU = resources->vkChosenGPU;
		vkDevice = resources->vkDevice;

		// Initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = vkChosenGPU;
		allocatorInfo.device = vkDevice;
		allocatorInfo.instance = vkInstance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		VmaVulkanFunctions vma_vulkan_func{};
		vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vma_vulkan_func.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

		allocatorInfo.pVulkanFunctions = &vma_vulkan_func;
		vmaCreateAllocator(&allocatorInfo, &vmaAllocator);

		HandlerDeletionQueue.push_back([&]() {
			vmaDestroyAllocator(vmaAllocator);
			});
	}
	~AllocationHandler() { Cleanup(); }

	void Cleanup()
	{
		HandlerDeletionQueue.flush();
	}

	void InitImmediateCommandPool()
	{
		VkCommandPoolCreateInfo commandPoolInfo = vkInit::CommandPool_CreateInfo(CoreResources->vkGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
		
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
			VK_CHECK(vkQueueSubmit2(CoreResources->GetCurrentFrame().vkGraphicsQueue, 1, &submit, vkImmediateFence));

		VK_CHECK(vkWaitForFences(vkDevice, 1, &vkImmediateFence, true, 9999999999));
	}

	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	AllocatedBuffer CreateGPUBuffer(size_t allocSize, VkBufferUsageFlags usage, bool addToDeletionQueue = false)
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

		if (addToDeletionQueue)
		{
			HandlerDeletionQueue.push_back([=]() {
				DestroyBuffer(newBuffer);
				});
		}

		return newBuffer;
	}

	AllocatedBuffer CreateGPUBufferWithAlignment(size_t allocSize, VkBufferUsageFlags usage, VkDeviceSize minAlignment, bool addToDeletionQueue = false)
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
		VK_CHECK(vmaCreateBufferWithAlignment(vmaAllocator,
			&bufferInfo,
			&vmaallocInfo,
			minAlignment,
			&newBuffer.buffer,
			&newBuffer.allocation,
			&newBuffer.info));

		if (addToDeletionQueue)
		{
			HandlerDeletionQueue.push_back([=]() {
				DestroyBuffer(newBuffer);
				});
		}

		return newBuffer;
	}

	// This assumes that Smart Access Memory / Resizable BAR is enabled!
	// No staging buffer is used as rebar should be enabled.
	AllocatedBuffer CreateAndPopulateGPUBuffer(size_t allocSize, const void* data, VkBufferUsageFlags usage, bool addToDeletionQueue = false)
	{
		// Create index buffer
		AllocatedBuffer newBuffer = CreateGPUBuffer(allocSize, usage, addToDeletionQueue);

		void* newBufferData = newBuffer.allocation->GetMappedData();

		// Copy vertex buffer
		memcpy(newBufferData, data, allocSize);

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
		T* data = static_cast<T*>(buffer.allocation->GetMappedData());
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
	GPUMeshBuffers UploadMeshToGPU(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{

		const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		GPUMeshBuffers newSurface;

		// Create vertex buffer
		newSurface.vertexBuffer = CreateGPUBuffer(vertexBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo vertexDeviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
		newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(vkDevice, &vertexDeviceAdressInfo);

		// Create index buffer
		newSurface.indexBuffer = CreateGPUBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo indexDeviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.indexBuffer.buffer };
		newSurface.indexBufferAddress = vkGetBufferDeviceAddress(vkDevice, &indexDeviceAdressInfo);

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
	GPURaytracingMeshBuffer UploadMeshForRaytracingToGPU(const std::vector<VertexPos>& vertices, const std::vector<uint32_t>& indices)
	{
		const size_t vertexBufferSize = vertices.size() * sizeof(VertexPos);
		const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

		GPURaytracingMeshBuffer newSurface;

		// Create vertex buffer
		newSurface.vertexBuffer = CreateGPUBuffer(vertexBufferSize,
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo vertexDeviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.vertexBuffer.buffer };
		newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(vkDevice, &vertexDeviceAdressInfo);

		// Create index buffer
		newSurface.indexBuffer = CreateGPUBuffer(indexBufferSize, 
			VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

		// Find the address of the vertex buffer
		VkBufferDeviceAddressInfo indexDeviceAdressInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,.buffer = newSurface.indexBuffer.buffer };
		newSurface.indexBufferAddress = vkGetBufferDeviceAddress(vkDevice, &indexDeviceAdressInfo);

		void* vertexData = newSurface.vertexBuffer.allocation->GetMappedData();
		void* indexData = newSurface.indexBuffer.allocation->GetMappedData();

		// Copy vertex buffer
		memcpy(vertexData, vertices.data(), vertexBufferSize);
		// Copy index buffer
		memcpy(indexData, indices.data(), indexBufferSize);

		// Build raytracing acceleration structure
		uint32_t maxPrimitiveCount = indices.size() / 3;

		VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.vertexData.deviceAddress = newSurface.vertexBufferAddress;
		geometry.geometry.triangles.vertexStride = sizeof(VertexPos);
		geometry.geometry.triangles.maxVertex = vertices.size() - 1;
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.indexData.deviceAddress = newSurface.indexBufferAddress;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{ };
		buildRangeInfo.firstVertex = 0;
		buildRangeInfo.primitiveCount = maxPrimitiveCount;
		buildRangeInfo.primitiveOffset = 0;
		buildRangeInfo.transformOffset = 0;

		newSurface.accelerationStructureGeometry = geometry;
		newSurface.accelerationStructureBuildRangeInfo = buildRangeInfo;

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
	AllocatedImage AllocateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped, bool addToDeletionQueue = true)
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

		if(addToDeletionQueue)
		{
			HandlerDeletionQueue.push_back([=]() {
			vkDestroyImageView(vkDevice, newImage.imageView, nullptr);
			vmaDestroyImage(vmaAllocator, newImage.image, newImage.allocation);
			});
		}

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

	void CreateAccelerationStructure(const VkAccelerationStructureCreateInfoKHR& asCreateInfo, VkAccelerationStructureKHR& accelerationStructure, bool addToDeletionQueue = false)
	{
		CoreResources->DispatchTable.fp_vkCreateAccelerationStructureKHR(CoreResources->vkDevice, &asCreateInfo, nullptr, &accelerationStructure);

		if (addToDeletionQueue)
		{
			HandlerDeletionQueue.push_back([=]() {
				CoreResources->DispatchTable.fp_vkDestroyAccelerationStructureKHR(vkDevice, accelerationStructure, nullptr);
				});
		}
	}

	void BuildAccelerationStructure(
		const VkAccelerationStructureBuildGeometryInfoKHR& asInfo, 
		VkAccelerationStructureBuildRangeInfoKHR* blas_ranges[],
		VkQueryPool& queryPool
	)
	{
		ImmediateSubmit([&](VkCommandBuffer cmd) 
		{
			vkCmdResetQueryPool(cmd, queryPool, 0, 1);
			CoreResources->DispatchTable.fp_vkCmdBuildAccelerationStructuresKHR(cmd, 1, &asInfo, blas_ranges);

		});
		ImmediateSubmit([&](VkCommandBuffer cmd)
		{
			CoreResources->DispatchTable.fp_vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, 1, &asInfo.dstAccelerationStructure,
			VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, 0);
		});
		
	}

	void CopyAccelerationStructure(VkAccelerationStructureKHR& src, VkAccelerationStructureKHR& dest, bool deleteSrc = false)
	{
		VkCopyAccelerationStructureInfoKHR copyInfo{ VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR };
		copyInfo.src = src;
		copyInfo.dst = dest;
		copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
		ImmediateSubmit([&](VkCommandBuffer cmd)
		{
				CoreResources->DispatchTable.fp_vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);
		});
		if (deleteSrc)
		{
			CoreResources->DispatchTable.fp_vkDestroyAccelerationStructureKHR(vkDevice, src, nullptr);
		}
	}

	void CopyRtHandleData(const AllocatedBuffer& rtSBTBuffer, std::vector<uint8_t>& handles, const uint32_t& handleSize,
		VkStridedDeviceAddressRegionKHR& raygenRegion, 
		VkStridedDeviceAddressRegionKHR& MissRegion, uint32_t& missCount,
		VkStridedDeviceAddressRegionKHR& HitRegion, uint32_t& hitCount)
	{
		// Helper to retrieve the handle data
		auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

		// Map the SBT buffer and write in the handles.
		auto* pSBTBuffer = reinterpret_cast<uint8_t*>(rtSBTBuffer.allocation->GetMappedData());
		uint8_t* pData{ nullptr };
		uint32_t handleIdx{ 0 };

		// Raygen
		pData = pSBTBuffer;
		memcpy(pData, getHandle(handleIdx++), handleSize);

		// Miss
		pData = pSBTBuffer + raygenRegion.size;
		for (uint32_t c = 0; c < missCount; c++)
		{
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += MissRegion.stride;
		}

		// Hit
		pData = pSBTBuffer + raygenRegion.size + MissRegion.size;
		for (uint32_t c = 0; c < hitCount; c++)
		{
			memcpy(pData, getHandle(handleIdx++), handleSize);
			pData += HitRegion.stride;
		}
	}

private:
	RenderCoreResources* CoreResources;
	RaytracingResources* RtResources;
	VmaAllocator vmaAllocator;

	// Needed vulkan handles
	VkInstance vkInstance;
	VkPhysicalDevice vkChosenGPU;
	VkDevice vkDevice;

	// Immediate submit structures
	VkFence vkImmediateFence;
	VkCommandBuffer vkImmediateCommandBuffer;
	VkCommandPool vkImmediateCommandPool;

	// Allocated resources deletion queue
	DeletionQueue HandlerDeletionQueue;
};