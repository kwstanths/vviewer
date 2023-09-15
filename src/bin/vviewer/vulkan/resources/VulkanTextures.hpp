#ifndef __VulkanTextures_hpp__
#define __VulkanTextures_hpp__

#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "utils/FreeList.hpp"

namespace vengine
{

/**
 * @brief A class to manage bindless textures
 *
 */
class VulkanTextures
{
public:
    VulkanTextures(VulkanContext &vkctx);

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &descriptorSetLayout() { return m_descriptorSetLayout; }
    VkDescriptorSet &descriptorSet() { return m_descriptorSet; }

    void updateTextures();

    std::shared_ptr<Texture> createTexture(std::string imagePath,
                                           VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                                           bool keepImage = false,
                                           bool addDescriptor = true);
    std::shared_ptr<Texture> createTexture(std::string id,
                                           std::shared_ptr<Image<stbi_uc>> image,
                                           VkFormat format = VK_FORMAT_R8G8B8A8_UNORM,
                                           bool addDescriptor = true);
    std::shared_ptr<Texture> createTextureHDR(std::string imagePath, bool keepImage = false);

    std::shared_ptr<Texture> addTexture(std::shared_ptr<Texture> tex);

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSet m_descriptorSet;

    std::vector<std::shared_ptr<VulkanTexture>> m_texturesToUpdate;
    vengine::FreeList m_freeList;

    VkResult createDescriptorSetsLayout(uint32_t nBindlessTextures);
    VkResult createDescriptorPool(uint32_t nImages, uint32_t nBindlessTextures);
    VkResult createDescriptorSets();
};

}  // namespace vengine

#endif