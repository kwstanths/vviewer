#ifndef __VulkanTextures_hpp__
#define __VulkanTextures_hpp__

#include "core/Textures.hpp"
#include "utils/FreeList.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"

namespace vengine
{

/**
 * @brief A class to manage bindless textures
 *
 */
class VulkanTextures : public Textures
{
public:
    VulkanTextures(VulkanContext &vkctx);

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &descriptorSetLayout() { return m_descriptorSetLayout; }
    VkDescriptorSet &descriptorSet() { return m_descriptorSet; }
    void createBaseTextures();

    void updateTextures();

    Texture *createTexture(const AssetInfo &info, ColorSpace colorSpace = ColorSpace::sRGB) override;
    Texture *createTexture(const Image<stbi_uc> &image) override;
    Texture *createTextureHDR(const AssetInfo &info) override;

    Texture *addTexture(Texture *tex);

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorSet m_descriptorSet;

    std::vector<VulkanTexture *> m_texturesToUpdate;
    vengine::FreeList m_freeList;

    VkResult createDescriptorSetsLayout(uint32_t nBindlessTextures);
    VkResult createDescriptorPool(uint32_t nImages, uint32_t nBindlessTextures);
    VkResult createDescriptorSets();
};

}  // namespace vengine

#endif