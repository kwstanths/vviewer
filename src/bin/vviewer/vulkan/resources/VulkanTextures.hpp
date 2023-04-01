#ifndef __VulkanTextures_hpp__
#define __VulkanTextures_hpp__

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "utils/FreeList.hpp"

class VulkanTextures {
public:
    VulkanTextures(VulkanContext& vkctx);

    bool initResources();
    bool initSwapchainResources(uint32_t nImages);

    bool releaseResources();
    bool releaseSwapchainResources();

    VkDescriptorSetLayout& layoutTextures() { return m_descriptorSetLayoutTextures; }
    VkDescriptorSet& descriptorTextures() { return m_descriptorSetBindlessTextures; }

    void updateTextures();

    std::shared_ptr<Texture> createTexture(std::string imagePath, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool keepImage = false, bool addDescriptor = true);
    std::shared_ptr<Texture> createTexture(std::string id, std::shared_ptr<Image<stbi_uc>> image, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, bool addDescriptor = true);
    std::shared_ptr<Texture> createTextureHDR(std::string imagePath, bool keepImage = false);

    std::shared_ptr<Texture> addTexture(std::shared_ptr<Texture> tex);

private:
    VulkanContext& m_vkctx;

    VkDescriptorPool m_descriptorPool;
    VkDescriptorSetLayout m_descriptorSetLayoutTextures;
    VkDescriptorSet m_descriptorSetBindlessTextures;

    std::vector<std::shared_ptr<VulkanTexture>> m_texturesToUpdate;
    FreeList m_freeList;

    bool createDescriptorSetsLayout(uint32_t nBindlessTextures);
    bool createDescriptorPool(uint32_t nImages, uint32_t nBindlessTextures);
    bool createDescriptorSets();
};

#endif