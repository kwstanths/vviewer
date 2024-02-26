#include "VulkanRandom.hpp"

#include "math/PMJSequences.hpp"
#include "math/BlueNoise.hpp"

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"

namespace vengine
{

VulkanRandom::VulkanRandom(VulkanContext &vkctx)
    : m_vkctx(vkctx)
{
}

VkResult VulkanRandom::initResources()
{
    VULKAN_CHECK_CRITICAL(createBuffers());
    VULKAN_CHECK_CRITICAL(createDescriptorSetLayout());
    VULKAN_CHECK_CRITICAL(createDescriptorPool());
    VULKAN_CHECK_CRITICAL(createDescriptorSet());
    VULKAN_CHECK_CRITICAL(updateDescriptorSet());

    return VK_SUCCESS;
}

VkResult VulkanRandom::releaseResources()
{
    m_storageBufferPMJSequences.destroy(m_vkctx.device());
    m_storageBufferBlueNoise.destroy(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRandom::createBuffers()
{
    /* Create a buffer to hold the pmj02bn sequences */
    VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                       m_vkctx.device(),
                                       PMJ02BN_TABLE_SIZE_BYTES,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       m_storageBufferPMJSequences));

    /* Create a buffer to hold the blue noise textures */
    VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                       m_vkctx.device(),
                                       BLUE_NOISE_TABLE_SIZE_BYTES,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       m_storageBufferBlueNoise));

    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_vkctx.device(), m_storageBufferPMJSequences.memory(), 0, VK_WHOLE_SIZE, 0, &data));
        memcpy(data, &pmj02bnSequences[0][0][0], PMJ02BN_TABLE_SIZE_BYTES);
        vkUnmapMemory(m_vkctx.device(), m_storageBufferPMJSequences.memory());
    }

    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_vkctx.device(), m_storageBufferBlueNoise.memory(), 0, VK_WHOLE_SIZE, 0, &data));
        memcpy(data, &bluNoiseTextures[0][0][0], BLUE_NOISE_TABLE_SIZE_BYTES);
        vkUnmapMemory(m_vkctx.device(), m_storageBufferBlueNoise.memory());
    }

    return VK_SUCCESS;
}

VkResult VulkanRandom::createDescriptorSetLayout()
{
    /* Create bindings for PMJ sequences and blue noise data */
    VkDescriptorSetLayoutBinding pmjlLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        0,
        1);
    VkDescriptorSetLayoutBinding blueNoiselLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        1,
        1);

    std::array<VkDescriptorSetLayoutBinding, 2> setBindings = {pmjlLayoutBinding, blueNoiselLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(setBindings.size()), setBindings.data());

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanRandom::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2);
    VkDescriptorPoolCreateInfo descriptorPoolInfo = vkinit::descriptorPoolCreateInfo(1, &poolSize, 1);

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &descriptorPoolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanRandom::createDescriptorSet()
{
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, 1, &m_descriptorSetLayout);
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, &m_descriptorSet));

    return VK_SUCCESS;
}

VkResult VulkanRandom::updateDescriptorSet()
{
    /* Update pmj02bn sequences buffer binding */
    VkDescriptorBufferInfo pmjDescriptor = vkinit::descriptorBufferInfo(m_storageBufferPMJSequences.buffer(), 0, VK_WHOLE_SIZE);
    VkWriteDescriptorSet pmjWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 1, &pmjDescriptor);

    /* Update blue noise buffer binding */
    VkDescriptorBufferInfo blueNoiseDescriptor = vkinit::descriptorBufferInfo(m_storageBufferBlueNoise.buffer(), 0, VK_WHOLE_SIZE);
    VkWriteDescriptorSet blueNoiseWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, &blueNoiseDescriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {pmjWrite, blueNoiseWrite};
    vkUpdateDescriptorSets(
        m_vkctx.device(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

    return VK_SUCCESS;
}

}  // namespace vengine