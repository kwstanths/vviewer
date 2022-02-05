#ifndef __VulkanDynamicUBO_hpp__
#define __VulkanDynamicUBO_hpp__

#include "IncludeVulkan.hpp"
#include "VulkanUtils.hpp"

/**
    A class to manage a dynamic uniform buffer that stores data of type Block
*/
template<typename Block>
class VulkanDynamicUBO {
    friend class VulkanRenderer;
public:
    VulkanDynamicUBO() {};

    /**
        Allocate cpu memory for nBlocks aligned based on the minUniformBufferOffsetAlignment
        @param minUniformBufferOffsetAlignment The device's minimum alignment
        @paramn nBlocks The numbers of blocks to allocate
    */
    bool init(uint32_t minUniformBufferOffsetAlignment, uint32_t nBlocks)
    {
        m_minUniformBufferOffsetAlignment = minUniformBufferOffsetAlignment;
        m_nBlocks = nBlocks;

        /* Calculate block alignment */
        m_blockAlignment = (sizeof(Block) + m_minUniformBufferOffsetAlignment - 1) & ~(m_minUniformBufferOffsetAlignment - 1);

        /* Allocate aligned memory for nBlocks */
        m_dataTransferSpace = (Block *)_aligned_malloc(m_blockAlignment * nBlocks, m_blockAlignment);
        isInited = true;
        return true;
    }

    /**
        Creates nBuffers GPU buffers to hold the allocated blocks, one buffer for each swapchain image
        @param physicalDevice The physical device
        @param device The logical device
        @param nBuffers The number of buffers to allocate
    */
    bool createBuffers(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t nBuffers)
    {
        m_buffers = nBuffers;

        /* Create GPU buffers */
        VkDeviceSize bufferSize = m_blockAlignment * m_nBlocks;
        m_buffer.resize(nBuffers);
        m_bufferMemory.resize(nBuffers);

        for (size_t i = 0; i < nBuffers; i++)
        {
            createBuffer(physicalDevice, device, bufferSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_buffer[i],
                m_bufferMemory[i]);
        }

        return true;
    }

    /**
        Get an allocated buffer
        @param index Buffer to get 
    */
    VkBuffer getBuffer(size_t index) const
    {
        assert(index < m_buffer.size());
        return m_buffer[index];
    }

    /**
        Get an allocated buffer's memory
        @param Buffer memory to get 
    */
    VkDeviceMemory getBufferMemory(size_t index) const
    {
        assert(index < m_bufferMemory.size());
        return m_bufferMemory[index];
    }

    /**
        Get the size of an aligned block
    */
    uint32_t getBlockSizeAligned() const
    {
        return m_blockAlignment;
    }

    /**
        Get the CPU memory address of a block 
    */
    Block * getBlock(size_t index) const
    {
        return (Block *)((uint64_t)m_dataTransferSpace + (index * m_blockAlignment));
    }

    /**
        Get total number of blocks allocated
    */
    uint32_t getNBlocks() const
    {
        return m_nBlocks;
    }

    /**
        Get number of gpu buffers created
    */
    uint32_t getNBuffers() const
    {
        return m_buffers;
    }

    /**
        Destroy the CPU memory
    */
    bool destroyCPUMemory()
    {
        if (isInited)
            _aligned_free(m_dataTransferSpace);

        return true;
    }

    /**
        Destroy GPU buffers
    */
    void destroyGPUBuffers(VkDevice device)
    {
        for (int i = 0; i < m_buffers; i++) {
            vkDestroyBuffer(device, getBuffer(i), nullptr);
            vkFreeMemory(device, getBufferMemory(i), nullptr);
        }
        m_buffers = 0;
    }

private:
    bool isInited = false;
    Block * m_dataTransferSpace = nullptr;

    uint32_t m_minUniformBufferOffsetAlignment;
    uint32_t m_blockAlignment;
    uint32_t m_nBlocks;
    uint32_t m_buffers = 0;

    std::vector<VkBuffer> m_buffer;
    std::vector<VkDeviceMemory> m_bufferMemory;
};

#endif

