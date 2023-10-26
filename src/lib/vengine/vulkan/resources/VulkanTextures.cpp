#include "VulkanTextures.hpp"

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
            auto vkTexture = std::static_pointer_cast<VulkanTexture>(itr->second);
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
    createTexture(std::make_shared<Image<stbi_uc>>("white", Color::WHITE, ColorSpace::LINEAR));
    createTexture(std::make_shared<Image<stbi_uc>>("whiteColor", Color::WHITE, ColorSpace::sRGB));
    createTexture(std::make_shared<Image<stbi_uc>>("normalmapdefault", Color::NORMAL_MAP, ColorSpace::LINEAR));
}

void VulkanTextures::updateTextures()
{
    std::vector<VkDescriptorImageInfo> imageInfos(m_texturesToUpdate.size());
    std::vector<VkWriteDescriptorSet> writeSets(m_texturesToUpdate.size());
    for (uint32_t i = 0; i < m_texturesToUpdate.size(); i++) {
        auto tex = m_texturesToUpdate[i];

        imageInfos[i] = vkinit::descriptorImageInfo(tex->sampler(), tex->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        writeSets[i] = vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfos[i]);
        writeSets[i].dstArrayElement = tex->getBindlessResourceIndex();
    }

    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_texturesToUpdate.clear();
}

std::shared_ptr<Texture> VulkanTextures::createTexture(std::string imagePath, ColorSpace colorSpace, bool keepImage)
{
    try {
        auto &imagesMap = AssetManager::getInstance().imagesCharMap();

        std::shared_ptr<Image<stbi_uc>> image;
        if (imagesMap.isPresent(imagePath)) {
            image = imagesMap.get(imagePath);
        } else {
            image = std::make_shared<Image<stbi_uc>>(imagePath, colorSpace);
            imagesMap.add(image);
        }

        auto temp = createTexture(image);

        if (!keepImage) {
            imagesMap.remove(imagePath);
        }

        return temp;
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleWarning("VulkanTextures::createTexture(): Can't create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanTextures::createTexture(std::shared_ptr<Image<stbi_uc>> image)
{
    try {
        auto &texturesMap = AssetManager::getInstance().texturesMap();

        if (texturesMap.isPresent(image->name())) {
            return texturesMap.get(image->name());
        }

        auto temp = std::make_shared<VulkanTexture>(
            image, m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsQueue(), m_vkctx.graphicsCommandPool(), true);

        addTexture(temp);

        return texturesMap.add(temp);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("VulkanTextures::createTexture(): Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanTextures::createTextureHDR(std::string imagePath, bool keepImage)
{
    try {
        /* Check if a texture has already been created for that image */
        auto &texturesMap = AssetManager::getInstance().texturesMap();
        if (texturesMap.isPresent(imagePath)) {
            return texturesMap.get(imagePath);
        }

        /* Check if the image has already been imported, if not read it from disk  */
        std::shared_ptr<Image<float>> image;
        auto &imagesMap = AssetManager::getInstance().imagesHDRMap();
        if (imagesMap.isPresent(imagePath)) {
            image = imagesMap.get(imagePath);
        } else {
            image = std::make_shared<Image<float>>(imagePath, ColorSpace::LINEAR);
            imagesMap.add(image);
        }

        /* Create texture for that image */
        auto temp = std::make_shared<VulkanTexture>(
            image, m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsQueue(), m_vkctx.graphicsCommandPool(), false);

        /* If you are not to keep the image in memory, remove from the assets */
        if (!keepImage) {
            imagesMap.remove(imagePath);
        }

        return texturesMap.add(temp);
    } catch (std::runtime_error &e) {
        debug_tools::ConsoleCritical("VulkanTextures::createTextureHDR(): Failed to create a vulkan HDR texture: " +
                                     std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanTextures::addTexture(std::shared_ptr<Texture> tex)
{
    auto vktex = std::static_pointer_cast<VulkanTexture>(tex);
    vktex->setBindlessResourceIndex(m_freeList.getFree());
    m_texturesToUpdate.push_back(vktex);
    return vktex;
}

VkResult VulkanTextures::createDescriptorSetsLayout(uint32_t nBindlessTextures)
{
    VkDescriptorSetLayoutBinding bindlessSamplersLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                           VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                           0,
                                           nBindlessTextures);
    VkDescriptorSetLayoutBinding bindlessStorageImagesLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1, nBindlessTextures);
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