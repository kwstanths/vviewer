#ifndef __VulkanMaterialSystem_hpp__
#define __VulkanMaterialSystem_hpp__

#include <vector>
#include <memory>

#include "core/io/AssimpLoadModel.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanUBO.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"
#include "vulkan/resources/VulkanTextures.hpp"
#include "vulkan/VulkanContext.hpp"

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

    VkDescriptorSetLayout &descriptorSetLayout() { return m_descriptorSetLayout; }
    VkDescriptorSet &descriptorSet(uint32_t index) { return m_descriptorSets[index]; }
    void updateDescriptor(uint32_t index);

    void updateBuffers(uint32_t index) const;
    VkBuffer getBuffer(uint32_t index);

    std::shared_ptr<Material> createMaterial(const std::string &name,
                                             const std::string &filepath,
                                             MaterialType type,
                                             bool createDescriptors = true);
    std::shared_ptr<Material> createMaterial(const std::string &name, MaterialType type, bool createDescriptors = true);
    std::shared_ptr<Material> createMaterialFromDisk(std::string name, std::string stackDirectory, VulkanTextures &textures);
    std::shared_ptr<Material> createZipMaterial(std::string name, std::string filename, VulkanTextures &textures);
    std::vector<std::shared_ptr<Material>> createImportedMaterials(const std::vector<ImportedMaterial> &importedMaterials,
                                                                   VulkanTextures &textures);

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;
    uint32_t m_swapchainImages = 0;

    VulkanUBO<MaterialData> m_materialsStorage;

    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;

    VkResult createDescriptorSetsLayout();
    VkResult createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps);
    VkResult createDescriptorSets(uint32_t nImages);
    void updateDescriptorSets();
};

}  // namespace vengine

#endif