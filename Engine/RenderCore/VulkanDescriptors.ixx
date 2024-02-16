module;
#include <vulkan/vulkan.h>
// Sometimes even standard importable libraries have to be included instead of imported to help intellisense.
#include <span>    

export module RenderCore.VulkanDescriptors;

import RenderCore.VulkanErrorLogger;
import <vector>;
import <deque>;



export class DescriptorAllocatorGrowable
{

public:

    struct PoolSizeRatio 
    {
        VkDescriptorType type;
        float ratio;
    };

    void Init(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
    {
        Ratios.clear();

        for (auto r : poolRatios) {
            Ratios.push_back(r);
        }

        VkDescriptorPool newPool = CreatePool(device, maxSets, poolRatios);

        SetsPerPool = maxSets * 1.5; // Grow the capacity of next pool allocation

        ReadyPools.push_back(newPool);
    }

    void ClearPools(VkDevice device)
    {
        for (auto p : ReadyPools) {
            vkResetDescriptorPool(device, p, 0);
        }
        for (auto p : FullPools) {
            vkResetDescriptorPool(device, p, 0);
            ReadyPools.push_back(p);
        }
        FullPools.clear();
    }

    void DestroyPools(VkDevice device)
    {
        for (auto p : ReadyPools) {
            vkDestroyDescriptorPool(device, p, nullptr);
        }
        ReadyPools.clear();
        for (auto p : FullPools) {
            vkDestroyDescriptorPool(device, p, nullptr);
        }
        FullPools.clear();
    }

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout)
    {
        // Get or create a pool to allocate from
        VkDescriptorPool poolToUse = GetPool(device);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.pNext = nullptr;
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = poolToUse;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet ds;
        VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

        // Allocation failed. Try again with new pool.
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL) {

            FullPools.push_back(poolToUse);

            poolToUse = GetPool(device);
            allocInfo.descriptorPool = poolToUse;

            VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
        }

        ReadyPools.push_back(poolToUse);
        return ds;
    }

private:

    VkDescriptorPool GetPool(VkDevice device)
    {
        VkDescriptorPool newPool;
        if (ReadyPools.size() != 0) {
            newPool = ReadyPools.back();
            ReadyPools.pop_back();
        }
        else {
            //need to create a new pool
            newPool = CreatePool(device, SetsPerPool, Ratios);

            SetsPerPool = SetsPerPool * 1.5;
            if (SetsPerPool > 4092) {
                SetsPerPool = 4092;
            }
        }

        return newPool;
    }

    VkDescriptorPool CreatePool(VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (PoolSizeRatio ratio : poolRatios) {
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = ratio.type,
                .descriptorCount = uint32_t(ratio.ratio * setCount)
                });
        }

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = 0;
        pool_info.maxSets = setCount;
        pool_info.poolSizeCount = (uint32_t)poolSizes.size();
        pool_info.pPoolSizes = poolSizes.data();

        VkDescriptorPool newPool;
        vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
        return newPool;
    }

    std::vector<PoolSizeRatio> Ratios;
    std::vector<VkDescriptorPool> FullPools;
    std::vector<VkDescriptorPool> ReadyPools;
    uint32_t SetsPerPool;

};


export class DescriptorWriter
{

public:
    void WriteImage(int binding, VkImageView image, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
    {
        VkDescriptorImageInfo& info = ImageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = sampler,
            .imageView = image,
            .imageLayout = layout
            });

        VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

        write.dstBinding = binding;
        write.dstSet = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = &info;

        Writes.push_back(write);
    }

    void WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
    {
        VkDescriptorBufferInfo& info = BufferInfos.emplace_back(VkDescriptorBufferInfo{
        .buffer = buffer,
        .offset = offset,
        .range = size
            });

        VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

        write.dstBinding = binding;
        write.dstSet = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pBufferInfo = &info;

        Writes.push_back(write);
    }

    void Clear()
    {
        ImageInfos.clear();
        Writes.clear();
        BufferInfos.clear();
    }

    void UpdateSet(VkDevice device, VkDescriptorSet set)
    {
        for (VkWriteDescriptorSet& write : Writes) {
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(device, (uint32_t)Writes.size(), Writes.data(), 0, nullptr);
    }

private:
    std::deque<VkDescriptorImageInfo> ImageInfos;
    std::deque<VkDescriptorBufferInfo> BufferInfos;
    std::vector<VkWriteDescriptorSet> Writes;
};


export class DescriptorLayoutBuilder 
{

public:

    void AddBinding(uint32_t binding, VkDescriptorType type)
    {
        VkDescriptorSetLayoutBinding newbind{};
        newbind.binding = binding;
        newbind.descriptorCount = 1;
        newbind.descriptorType = type;

        bindings.push_back(newbind);
    }

    void Clear()
    {
        bindings.clear();
    }

    VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages)
    {
        for (auto& b : bindings) {
            b.stageFlags |= shaderStages;
        }

        VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        info.pNext = nullptr;

        info.pBindings = bindings.data();
        info.bindingCount = (uint32_t)bindings.size();
        info.flags = 0;

        VkDescriptorSetLayout set;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

        return set;
    }

private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

export class DescriptorAllocator 
{

public:
    struct PoolSizeRatio 
    {
        VkDescriptorType type;
        float ratio;
    };

    void InitPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (PoolSizeRatio ratio : poolRatios) {
            poolSizes.push_back(VkDescriptorPoolSize{
                .type = ratio.type,
                .descriptorCount = uint32_t(ratio.ratio * maxSets)
                });
        }

        VkDescriptorPoolCreateInfo pool_info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool_info.flags = 0;
        pool_info.maxSets = maxSets;
        pool_info.poolSizeCount = (uint32_t)poolSizes.size();
        pool_info.pPoolSizes = poolSizes.data();

        vkCreateDescriptorPool(device, &pool_info, nullptr, &pool);
    }

    void ClearDescriptors(VkDevice device)
    {
        vkResetDescriptorPool(device, pool, 0);
    }

    void DestroyPool(VkDevice device)
    {
        vkDestroyDescriptorPool(device, pool, nullptr);
    }

    VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout)
    {
        VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet ds;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));

        return ds;
    }

private:
    VkDescriptorPool pool;
};
