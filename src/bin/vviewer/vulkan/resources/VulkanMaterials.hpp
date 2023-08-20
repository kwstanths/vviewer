#ifndef __VulkanMaterialSystem_hpp__
#define __VulkanMaterialSystem_hpp__

#include <vector>
#include <memory>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/resources/VulkanUBO.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"

namespace vengine
{

class VulkanMaterials
{
public:
    VulkanMaterials(VulkanContext &ctx);

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &layoutMaterial() { return m_descriptorSetLayoutMaterial; }
    VkDescriptorSet &descriptor(uint32_t index) { return m_descriptorSets[index]; }
    void updateDescriptor(uint32_t index);

    void updateBuffers(uint32_t index) const;
    VkBuffer getBuffer(uint32_t index);

    std::shared_ptr<Material> createMaterial(std::string name, MaterialType type, bool createDescriptors = true);

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;
    uint32_t m_swapchainImages = 0;

    VulkanUBO<MaterialData> m_materialsStorage;

    VkDescriptorSetLayout m_descriptorSetLayoutMaterial;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkResult createDescriptorSetsLayout();
    VkResult createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps);
    VkResult createDescriptorSets(uint32_t nImages);
    void updateDescriptorSets();
};

}  // namespace vengine

#endif