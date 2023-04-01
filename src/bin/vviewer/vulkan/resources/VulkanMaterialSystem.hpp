#ifndef __VulkanMaterialSystem_hpp__
#define __VulkanMaterialSystem_hpp__

#include <vector>
#include <memory>

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanUBO.hpp"

class VulkanMaterialSystem {
public:
    VulkanMaterialSystem(VulkanContext& ctx);

    bool initResources();
    bool initSwapchainResources(uint32_t nImages);

    bool releaseResources();
    bool releaseSwapchainResources();

    VkDescriptorSetLayout& layoutMaterial() { return m_descriptorSetLayoutMaterial; }
    VkDescriptorSet& descriptor(uint32_t index) { return m_descriptorSets[index]; }
    void updateDescriptor(uint32_t index);

    void updateBuffers(uint32_t index) const;
    VkBuffer getBuffer(uint32_t index);

    std::shared_ptr<Material> createMaterial(std::string name, MaterialType type, bool createDescriptors = true);

private:
    VulkanContext& m_vkctx;

    VkDescriptorPool m_descriptorPool;
    uint32_t m_swapchainImages = 0;

    VulkanUBO<MaterialData> m_materialsStorage;
    
    VkDescriptorSetLayout m_descriptorSetLayoutMaterial;
    std::vector<VkDescriptorSet> m_descriptorSets;

    bool createDescriptorSetsLayout();
    bool createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps);
    bool createDescriptorSets(uint32_t nImages);
    bool updateDescriptorSets();
};

#endif