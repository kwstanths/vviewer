#ifndef __VulkanBufferObject_hpp__
#define __VulkanBufferObject_hpp__

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/resources/VulkanBuffer.hpp"

#include <math/MathUtils.hpp>

namespace vengine
{

/**
    A class to create aligned CPU memory used as transfer space and map into a group of GPU buffers
*/
template <typename Block>
class VulkanBufferObject
{
public:
    VulkanBufferObject(uint32_t nBlocks) { m_nBlocks = nBlocks; };

    VulkanBufferObject(VulkanBufferObject const &) = delete;
    void operator=(VulkanBufferObject const &) = delete;

    /**
        Allocate cpu memory for nBlocks aligned based on the minBufferOffsetAlignment
        @param minBufferOffsetAlignment The device's minimum alignment
        @paramn nBlocks The numbers of blocks to allocate
    */
    VkResult init(uint32_t minBufferOffsetAlignment)
    {
        m_minBufferOffsetAlignment = minBufferOffsetAlignment;

        /* Calculate block alignment */
        m_blockAlignment = (sizeof(Block) + m_minBufferOffsetAlignment - 1) & static_cast<uint32_t>(~(m_minBufferOffsetAlignment - 1));

        /* Allocate aligned memory for m_nBlocks */
        /* std::aligned_alloc is not defined in Visual Studio compiler */
#ifndef _MSC_VER
        m_dataTransferSpace = (Block *)std::aligned_alloc(m_blockAlignment, m_blockAlignment * m_nBlocks);
#else
        m_blockAlignment = roundPow2(m_blockAlignment);
        m_dataTransferSpace = (Block *)_aligned_malloc(m_blockAlignment * m_nBlocks, m_blockAlignment);
#endif  // !_MSC_VER

        if (sizeof(Block) != blockSizeAligned()) {
            debug_tools::ConsoleWarning(
                "VulkanBufferObject::init(): Block size allocated is different than the one requested. Block type needs padding: " +
                std::to_string(sizeof(Block)) + " != " + std::to_string(blockSizeAligned()));
        }

        isInited = true;
        return VK_SUCCESS;
    }

    /**
        Creates nBuffers GPU buffers to hold the allocated blocks, one buffer for each swapchain image
        @param physicalDevice The physical device
        @param device The logical device
        @param nBuffers The number of buffers to allocate
    */
    VkResult createBuffers(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t nBuffers, VkBufferUsageFlags usageFlags)
    {
        m_buffers = nBuffers;

        /* Create GPU buffers */
        VkDeviceSize bufferSize = m_blockAlignment * m_nBlocks;
        m_gpuBuffers.resize(nBuffers);

        for (size_t i = 0; i < nBuffers; i++) {
            VULKAN_CHECK_CRITICAL(createBuffer(physicalDevice,
                                               device,
                                               bufferSize,
                                               usageFlags,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                               m_gpuBuffers[i]));
        }

        return VK_SUCCESS;
    }

    /**
     * @brief Update a buffer
     *
     * @param device
     * @param index
     */
    void updateBuffer(VkDevice device, uint32_t bufferIndex) const
    {
        void *data;
        vkMapMemory(device, memory(bufferIndex), 0, m_blockAlignment * m_nBlocks, 0, &data);
        memcpy(data, block(0), m_blockAlignment * m_nBlocks);
        vkUnmapMemory(device, memory(bufferIndex));
    }

    /**
     * @brief Update a buffer
     *
     * @param device
     * @param index
     */
    void updateBuffer(VkDevice device, uint32_t bufferIndex, uint32_t startIndex, uint32_t stopIndex) const
    {
        void *data;
        vkMapMemory(device, memory(bufferIndex), startIndex * m_blockAlignment, m_blockAlignment * (stopIndex - startIndex), 0, &data);
        memcpy(data, block(startIndex), m_blockAlignment * (stopIndex - startIndex));
        vkUnmapMemory(device, memory(bufferIndex));
    }

    /**
        Get an allocated buffer
        @param index Buffer to get
    */
    VkBuffer buffer(size_t index) const
    {
        assert(index < m_gpuBuffers.size());
        return m_gpuBuffers[index].buffer();
    }

    /**
        Get an allocated buffer's memory
        @param Buffer memory to get
    */
    VkDeviceMemory memory(size_t index) const
    {
        assert(index < m_gpuBuffers.size());
        return m_gpuBuffers[index].memory();
    }

    /**
        Get the size of an aligned block
    */
    uint32_t blockSizeAligned() const { return m_blockAlignment; }

    /**
        Get the CPU memory address of a block
    */
    Block *block(size_t index) const { return (Block *)((uint64_t)m_dataTransferSpace + (index * m_blockAlignment)); }

    /**
        Get total number of blocks allocated
    */
    uint32_t nblocks() const { return m_nBlocks; }

    /**
        Get number of gpu buffers created
    */
    uint32_t nbuffers() const { return m_buffers; }

    /**
        Destroy the CPU memory
    */
    void destroyCPUMemory()
    {
        if (isInited) {
#ifndef _MSC_VER
            free(m_dataTransferSpace);
#else
            _aligned_free(m_dataTransferSpace);
#endif  // !_MSC_VER
        }
    }

    /**
        Destroy GPU buffers
    */
    void destroyGPUBuffers(VkDevice device)
    {
        for (size_t i = 0U; i < m_buffers; i++) {
            vkDestroyBuffer(device, buffer(i), nullptr);
            vkFreeMemory(device, memory(i), nullptr);
        }
        m_buffers = 0;
    }

protected:
    Block *m_dataTransferSpace = nullptr;

private:
    bool isInited = false;

    uint32_t m_minBufferOffsetAlignment;
    uint32_t m_blockAlignment;
    uint32_t m_nBlocks;
    uint32_t m_buffers = 0;

    std::vector<VulkanBuffer> m_gpuBuffers;
};

}  // namespace vengine

#endif
