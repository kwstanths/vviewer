#ifndef __VulkanInstances_hpp__
#define __VulkanInstances_hpp__

#include "core/Instances.hpp"
#include "VulkanContext.hpp"
#include "vulkan/resources/VulkanSSBO.hpp"
#include "vulkan/resources/VulkanUBOAccessors.hpp"

namespace vengine
{

class VulkanScene;

class VulkanInstancesManager : public InstancesManager
{
    friend class VulkanScene;

public:
    VulkanInstancesManager(VulkanContext &vkctx, VulkanScene *scene);
    ~VulkanInstancesManager();

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &layoutInstanceData() { return m_descriptorSetLayoutInstanceData; }
    VkDescriptorSet &descriptorSetInstanceData(uint32_t imageIndex) { return m_descriptorSetsInstanceData[imageIndex]; };

    void build() override;

    void updateBuffers(uint32_t imageIndex);

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;

    /* Buffers for InstanceData */
    VulkanSSBO<InstanceData> m_instancesSSBO;
    VkDescriptorSetLayout m_descriptorSetLayoutInstanceData;
    std::vector<VkDescriptorSet> m_descriptorSetsInstanceData;

    /* Buffers for LightInstance */
    VulkanUBODefault<LightInstance> m_lightInstancesUBO;

    void initInstanceData(InstanceData *instanceData, SceneObject *so) override;

    VkResult createDescriptorSetsLayouts();
    VkResult createDescriptorPool(uint32_t nImages);
    VkResult createDescriptorSets(uint32_t nImages);
};

}  // namespace vengine

#endif