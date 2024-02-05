#ifndef __VulkanUBOAccessors_hpp__
#define __VulkanUBOAccessors_hpp__

#include <span>

#include "VulkanUBO.hpp"
#include "vengine/utils/FreeList.hpp"
#include "vengine/utils/InternCache.hpp"

namespace vengine
{

/**
 * @brief A class to manage a uniform buffer object with free access to blocks
 *
 * @tparam B
 */
template <typename B>
class VulkanUBODefault : public VulkanUBO<B>, public FreeList
{
public:
    VulkanUBODefault(uint32_t nBlocks)
        : VulkanUBO<B>(nBlocks)
        , FreeList(nBlocks){};

    class Block
    {
    public:
        Block(VulkanUBODefault<B> &ubo)
            : m_ubo(ubo)
        {
            /* Get an index for a free block in the ubo */
            m_blockIndex = static_cast<uint32_t>(m_ubo.getFree());
            /* Get pointer to free block */
            m_block = m_ubo.block(m_blockIndex);
        }

        ~Block()
        {
            /* Remove block index from the buffers */
            m_ubo.setFree(m_blockIndex);
        }

        const uint32_t &UBOBlockIndex() const { return m_blockIndex; }

    protected:
        VulkanUBODefault<B> &m_ubo;
        uint32_t m_blockIndex = -1;
        B *m_block = nullptr;
    };

private:
};

/**
 * @brief A class to manage a uniform buffer object and cached access to blocks
 *
 * @tparam B Will be used as a key to an unorderd map
 */
template <typename B>
class VulkanUBOCached : public VulkanUBO<B>, public InternCache<B>
{
public:
    VulkanUBOCached(uint32_t nBlocks)
        : VulkanUBO<B>(nBlocks)
        , InternCache<B>(){};

    VkResult init(uint32_t minUniformBufferOffsetAlignment)
    {
        VkResult res = VulkanUBO<B>::init(minUniformBufferOffsetAlignment);

        if (res == VK_SUCCESS) {
            InternCache<B>::setData(std::span<B>(VulkanUBO<B>::m_dataTransferSpace, VulkanUBO<B>::nblocks()));
        }

        return res;
    }

    class Block
    {
    public:
        Block(VulkanUBOCached<B> &ubo, const B &b)
            : m_ubo(ubo)
        {
            getBlock(b);
            *m_block = b;
        }

        void updateValue(const B &b)
        {
            /* If the value is the same don't do anything */
            if (b == *m_block) {
                return;
            }

            /* Remove old value */
            m_ubo.remove(m_blockIndex);

            /* Get new block */
            getBlock(b);
            *m_block = b;
        }

        const B *value() { return m_block; }

        ~Block()
        {
            /* Remove block index from the buffers */
            m_ubo.remove(m_blockIndex);
        }

        const uint32_t &UBOBlockIndex() const { return m_blockIndex; }

    private:
        VulkanUBOCached<B> &m_ubo;
        uint32_t m_blockIndex = -1;
        B *m_block = nullptr;

        void getBlock(const B &b)
        {
            /* Get a block index given that we want to store value b */
            m_blockIndex = static_cast<uint32_t>(m_ubo.lookupAndAdd(b));
            /* Get pointer to block */
            m_block = m_ubo.block(m_blockIndex);
        }
    };

private:
};

}  // namespace vengine

#endif