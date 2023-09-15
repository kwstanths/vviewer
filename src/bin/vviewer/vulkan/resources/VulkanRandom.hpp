#ifndef __VulkanRandom_hpp__
#define __VulkanRandom_hpp__

#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanBuffer.hpp"

namespace vengine
{

/**
 * @brief A class to manage the resources for random numbers
 *
 */
class VulkanRandom
{
public:
    VulkanRandom(VulkanContext &vkctx);

    VkResult initResources();
    VkResult releaseResources();

    VkDescriptorSetLayout &descriptorSetLayout() { return m_descriptorSetLayout; }
    VkDescriptorSet &descriptorSet() { return m_descriptorSet; }

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSet m_descriptorSet;

    VulkanBuffer m_storageBufferPMJSequences, m_storageBufferBlueNoise;

    VkResult createBuffers();
    VkResult createDescriptorSetLayout();
    VkResult createDescriptorPool();
    VkResult createDescriptorSet();
    VkResult updateDescriptorSet();
};

}  // namespace vengine

#endif