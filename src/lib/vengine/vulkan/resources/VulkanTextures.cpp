#include "VulkanTextures.hpp"

#include <array>

#include <debug_tools/Console.hpp>

#include "core/AssetManager.hpp"

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanTextures::VulkanTextures(VulkanContext &vkctx)
    : m_vkctx(vkctx)
    , m_freeList(VULKAN_LIMITS_MAX_TEXTURES)
{
}

VkResult VulkanTextures::initResources()
{
    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout(VULKAN_LIMITS_MAX_TEXTURES));

    createBaseTextures();

    return VK_SUCCESS;
}

VkResult VulkanTextures::initSwapchainResources(uint32_t nImages)
{
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages, VULKAN_LIMITS_MAX_TEXTURES));
    VULKAN_CHECK_CRITICAL(createDescriptorSets());

    /* Update all textures descriptors */
    m_texturesToUpdate.clear();
    auto &textures = AssetManager::getInstance().texturesMap();
    for (auto &t : textures) {
        m_texturesToUpdate.push_back(static_cast<VulkanTexture *>(t.second));
    }
    updateTextures();

    return VK_SUCCESS;
}

VkResult VulkanTextures::releaseResources()
{
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayout, nullptr);

    /* Destroy texture data */
    {
        auto &texturesMap = AssetManager::getInstance().texturesMap();
        for (auto itr = texturesMap.begin(); itr != texturesMap.end(); ++itr) {
            auto vkTexture = static_cast<VulkanTexture *>(itr->second);
            vkTexture->destroy(m_vkctx.device());
        }
        texturesMap.reset();
    }

    return VK_SUCCESS;
}

VkResult VulkanTextures::releaseSwapchainResources()
{
    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

void VulkanTextures::createBaseTextures()
{
    createTexture(Image<stbi_uc>(AssetInfo("white", AssetSource::ENGINE), Color::WHITE, ColorSpace::LINEAR));
    createTexture(Image<stbi_uc>(AssetInfo("whiteColor", AssetSource::ENGINE), Color::WHITE, ColorSpace::sRGB));
    createTexture(Image<stbi_uc>(AssetInfo("normalmapdefault", AssetSource::ENGINE), Color::NORMAL_MAP, ColorSpace::LINEAR));
}

void VulkanTextures::updateTextures()
{
    std::vector<VkDescriptorImageInfo> imageInfos(m_texturesToUpdate.size());
    std::vector<VkWriteDescriptorSet> writeSets(m_texturesToUpdate.size());
    for (uint32_t i = 0; i < m_texturesToUpdate.size(); i++) {
        auto tex = m_texturesToUpdate[i];

        imageInfos[i] = vkinit::descriptorImageInfo(tex->sampler(), tex->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        writeSets[i] = vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfos[i]);
        writeSets[i].dstArrayElement = tex->bindlessResourceIndex();
    }

    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_texturesToUpdate.clear();
}

Texture *VulkanTextures::createTexture(const AssetInfo &info, ColorSpace colorSpace)
{
    try {
        Image<stbi_uc> image(info, colorSpace);
        auto temp = createTexture(image);
        return temp;
    } catch (std::exception &e) {
        debug_tools::ConsoleWarning("VulkanTextures::createTexture(): Unable to create texture: " + std::string(e.what()));
        return nullptr;
    }
}

Texture *VulkanTextures::createTexture(const Image<stbi_uc> &image)
{
    auto &texturesMap = AssetManager::getInstance().texturesMap();
    if (texturesMap.has(image.name())) {
        return texturesMap.get(image.name());
    }

    auto temp = new VulkanTexture(
        image,
        {m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.queueManager().graphicsQueue()},
        true);

    addTexture(temp);

    return texturesMap.add(temp);
}

Texture *VulkanTextures::createTextureHDR(const AssetInfo &info)
{
    auto &texturesMap = AssetManager::getInstance().texturesMap();
    if (texturesMap.has(info.name)) {
        return texturesMap.get(info.name);
    }

    Image<float> image(info, ColorSpace::LINEAR);

    auto temp = new VulkanTexture(
        image,
        {m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.queueManager().graphicsQueue()},
        false);

    return texturesMap.add(temp);
}

Texture *VulkanTextures::addTexture(Texture *tex)
{
    auto vktex = static_cast<VulkanTexture *>(tex);
    vktex->bindlessResourceIndex() = m_freeList.getFree();
    m_texturesToUpdate.push_back(vktex);
    return vktex;
}

VkResult VulkanTextures::createDescriptorSetsLayout(uint32_t nBindlessTextures)
{
    VkDescriptorSetLayoutBinding bindlessSamplersLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
                                               VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                                           0,
                                           nBindlessTextures);
    VkDescriptorSetLayoutBinding bindlessStorageImagesLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
        1,
        nBindlessTextures);
    std::array<VkDescriptorSetLayoutBinding, 2> setBindings = {bindlessSamplersLayoutBinding, bindlessStorageImagesLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(setBindings.size()), setBindings.data());
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

    VkDescriptorBindingFlags bindlessFlags =
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    std::array<VkDescriptorBindingFlags, 2> binding_flags = {bindlessFlags, bindlessFlags};
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo = {};
    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    extendedInfo.bindingCount = static_cast<uint32_t>(binding_flags.size());
    extendedInfo.pBindingFlags = binding_flags.data();
    extendedInfo.pNext = nullptr;

    layoutInfo.pNext = &extendedInfo;

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayout));

    return VK_SUCCESS;
}

VkResult VulkanTextures::createDescriptorPool(uint32_t nImages, uint32_t nBindlessTextures)
{
    VkDescriptorPoolSize bindlessSamplersPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nBindlessTextures);
    VkDescriptorPoolSize bindlessStorageImagesPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nBindlessTextures);
    std::array<VkDescriptorPoolSize, 2> poolSizes = {bindlessSamplersPoolSize, bindlessStorageImagesPoolSize};

    VkDescriptorPoolCreateInfo poolInfo = vkinit::descriptorPoolCreateInfo(
        static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), static_cast<uint32_t>(nImages + nBindlessTextures * 2));
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanTextures::createDescriptorSets()
{
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, 1, &m_descriptorSetLayout);

    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, &m_descriptorSet));

    return VK_SUCCESS;
}

}  // namespace vengine