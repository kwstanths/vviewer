#ifndef __VulkanQueues_hpp__
#define __VulkanQueues_hpp__

#include <vector>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanStructs.hpp"

namespace vengine
{

class VulkanContext;

class VulkanQueueManager
{
public:
    VulkanQueueManager();
    ~VulkanQueueManager();

    VkResult init(VulkanContext& context);
    std::vector<VkDeviceQueueCreateInfo> getQueueCreateInfo();
    VkResult createQueues(VulkanContext &context);

    const std::vector<VulkanQueueFamilyInfo> &queueFamilies() const { return m_queueFamilies; }
    VkQueue &queue(uint32_t family, uint32_t index) { return m_queues[family][index]; }

    VkQueue &graphicsQueue();
    const std::pair<uint32_t, uint32_t> &graphicsQueueIndex() const { return m_graphicsQueue; }
    VkQueue &renderQueue();
    const std::pair<uint32_t, uint32_t> &renderQueueIndex() const { return m_renderQueue; }
    VkQueue &presentQueue();
    const std::pair<uint32_t, uint32_t> &presentQueueIndex() const { return m_presentQueue; }

private:
    std::vector<VulkanQueueFamilyInfo> m_queueFamilies;
    std::vector<std::vector<float>> m_queuePriorities;
    std::vector<std::vector<VkQueue>> m_queues;

    std::pair<uint32_t, uint32_t> m_graphicsQueue, m_renderQueue, m_presentQueue;
};

}  // namespace vengine

#endif
