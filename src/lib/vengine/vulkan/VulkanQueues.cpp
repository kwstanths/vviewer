#include "VulkanQueues.hpp"

#include "debug_tools/Console.hpp"

#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/VulkanContext.hpp"

namespace vengine
{

VulkanQueueManager::VulkanQueueManager()
{
}

VulkanQueueManager::~VulkanQueueManager()
{
}

VkResult VulkanQueueManager::init(VulkanContext &context)
{
    m_queueFamilies = findQueueFamilies(context.physicalDevice(), context.surface());

    return VK_SUCCESS;
}

std::vector<VkDeviceQueueCreateInfo> VulkanQueueManager::getQueueCreateInfo()
{
    m_queuePriorities.resize(m_queueFamilies.size());
    for (uint32_t i = 0; i < m_queueFamilies.size(); i++) {
        const VulkanQueueFamilyInfo &queueFamily = m_queueFamilies[i];
        m_queuePriorities[i].resize(queueFamily.properties.queueCount);
    }

    std::vector<VkDeviceQueueCreateInfo> createInfos;
    for (uint32_t i = 0; i < m_queueFamilies.size(); i++) {
        const VulkanQueueFamilyInfo &queueFamily = m_queueFamilies[i];
        float *priorities = m_queuePriorities[i].data();
        memset(priorities, 0, sizeof(float) * queueFamily.properties.queueCount);

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily.index;
        queueCreateInfo.queueCount = queueFamily.properties.queueCount;
        queueCreateInfo.pQueuePriorities = priorities;
        createInfos.push_back(queueCreateInfo);
    }
    return createInfos;
}

VkResult VulkanQueueManager::createQueues(VulkanContext &context)
{
    if (m_queues.size() != 0)
        return VK_ERROR_UNKNOWN;

    m_queues.resize(m_queueFamilies.size());
    bool foundGraphics = false;
    bool foundRender = false;
    bool foundPresent = false;
    for (uint32_t f = 0; f < m_queueFamilies.size(); f++) {
        VulkanQueueFamilyInfo &queueFamily = m_queueFamilies[f];
        m_queues[f].resize(queueFamily.properties.queueCount);

        if (queueFamily.hasGraphics() && queueFamily.properties.queueCount > 1)
        {
            m_graphicsQueue.first = f;
            m_graphicsQueue.second = 0;
            m_renderQueue.first = f;
            m_renderQueue.second = 1;
            foundGraphics = true;
            foundRender = true;
        }

        if (queueFamily.hasPresent()) {
            m_presentQueue.first = f;
            m_presentQueue.second = 0;
            foundPresent = true;
        }

        for (uint32_t q = 0; q < queueFamily.properties.queueCount; q++) {
            vkGetDeviceQueue(context.device(), queueFamily.index, q, &m_queues[f][q]);
        }
    }

    if (!foundGraphics || !foundPresent || !foundRender)
        return VK_ERROR_UNKNOWN;

    return VK_SUCCESS;
}

VkQueue &VulkanQueueManager::graphicsQueue()
{
    return m_queues[m_graphicsQueue.first][m_graphicsQueue.second];
}

VkQueue &VulkanQueueManager::renderQueue()
{
    return m_queues[m_renderQueue.first][m_renderQueue.second];
}

VkQueue &VulkanQueueManager::presentQueue()
{
    return m_queues[m_presentQueue.first][m_presentQueue.second];
}

}  // namespace vengine