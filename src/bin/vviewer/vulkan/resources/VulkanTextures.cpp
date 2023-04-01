#include "VulkanTextures.hpp"

#include <utils/Console.hpp>

#include "core/AssetManager.hpp"

#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanLimits.hpp"

VulkanTextures::VulkanTextures(VulkanContext& vkctx) : m_vkctx(vkctx), m_freeList(VULKAN_LIMITS_MAX_TEXTURES) 
{

}

bool VulkanTextures::initResources()
{
    createDescriptorSetsLayout(VULKAN_LIMITS_MAX_TEXTURES);

    createTexture("white", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_UNORM);
    createTexture("whiteColor", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::WHITE), VK_FORMAT_R8G8B8A8_SRGB);
    createTexture("normalmapdefault", std::make_shared<Image<stbi_uc>>(Image<stbi_uc>::Color::NORMAL_MAP), VK_FORMAT_R8G8B8A8_UNORM);

    return true;
}

bool VulkanTextures::initSwapchainResources(uint32_t nImages)
{
    bool ret = createDescriptorPool(nImages, VULKAN_LIMITS_MAX_TEXTURES);
    ret = ret && createDescriptorSets();

    updateTextures();

    return true;
}

bool VulkanTextures::releaseResources()
{
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutTextures, nullptr);

    /* Destroy texture data */
    {
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto vkTexture = std::static_pointer_cast<VulkanTexture>(itr->second);
            vkTexture->destroy(m_vkctx.device());
        }
        instance.reset();
    }
    
    return true;
}

bool VulkanTextures::releaseSwapchainResources()
{
    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return true;
}

void VulkanTextures::updateTextures()
{
    std::vector<VkDescriptorImageInfo> imageInfos(m_texturesToUpdate.size());
    std::vector<VkWriteDescriptorSet> writeSets(m_texturesToUpdate.size());
    for(uint32_t i = 0; i < m_texturesToUpdate.size(); i++)
    {
        auto tex = m_texturesToUpdate[i];

        VkWriteDescriptorSet& descriptorWrite = writeSets[i];
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.dstArrayElement = tex->getBindlessResourceIndex();
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.dstSet = m_descriptorSetBindlessTextures;
        descriptorWrite.dstBinding = 0;

        VkDescriptorImageInfo& imageInfo = imageInfos[i];
        imageInfo.sampler = tex->getSampler();
        imageInfo.imageView = tex->getImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        descriptorWrite.pImageInfo = &imageInfo;
    }

    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_texturesToUpdate.clear();
}

std::shared_ptr<Texture> VulkanTextures::createTexture(std::string imagePath, VkFormat format, bool keepImage, bool addDescriptor)
{
    try {
        AssetManager<std::string, Image<stbi_uc>>& instance = AssetManager<std::string, Image<stbi_uc>>::getInstance();
        
        std::shared_ptr<Image<stbi_uc>> image;
        if (instance.isPresent(imagePath)) {
            image = instance.get(imagePath);
        }
        else {
            image = std::make_shared<Image<stbi_uc>>(imagePath);
            instance.add(imagePath, image);
        }

        auto temp = createTexture(imagePath, image, format, addDescriptor);

        if (!keepImage) {
            instance.remove(imagePath);
        }

        return temp;
    } catch (std::runtime_error& e) {
        utils::ConsoleWarning("VulkanTextures::createTexture(): Can't create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanTextures::createTexture(std::string id, std::shared_ptr<Image<stbi_uc>> image, VkFormat format, bool addDescriptor)
{
    try {
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        
        if (instance.isPresent(id)) {
            return instance.get(id);
        }

        TextureType type = TextureType::NO_TYPE;
        if (format == VK_FORMAT_R8G8B8A8_SRGB) type = TextureType::COLOR;
        else if (format == VK_FORMAT_R8G8B8A8_UNORM) type = TextureType::LINEAR;

        auto temp = std::make_shared<VulkanTexture>(id, 
            image, 
            type, 
            m_vkctx.physicalDevice(), 
            m_vkctx.device(), 
            m_vkctx.graphicsQueue(), 
            m_vkctx.graphicsCommandPool(), 
            format, 
            true);

        if (addDescriptor)
        {
            addTexture(temp);
        }

        return instance.add(id, temp);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("VulkanTextures::createTexture(): Failed to create a vulkan texture: " + std::string(e.what()));
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Texture> VulkanTextures::createTextureHDR(std::string imagePath, bool keepImage)
{
    try {
        /* Check if an hdr texture has already been created for that image */
        AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
        if (instance.isPresent(imagePath)) {
            return instance.get(imagePath);
        }
        
        /* Check if the image has already been imported, if not read it from disk  */
        std::shared_ptr<Image<float>> image;
        AssetManager<std::string, Image<float>>& images = AssetManager<std::string, Image<float>>::getInstance();
        if (images.isPresent(imagePath)) {
            image = images.get(imagePath);
        }
        else {
            image = std::make_shared<Image<float>>(imagePath);
            images.add(imagePath, image);
        }

        /* Create texture for that image */
        auto temp = std::make_shared<VulkanTexture>(imagePath, 
            image, 
            TextureType::HDR, 
            m_vkctx.physicalDevice(), 
            m_vkctx.device(), 
            m_vkctx.graphicsQueue(), 
            m_vkctx.graphicsCommandPool(),
            false);

        /* If you are not to keep the image in memory, remove from the assets */
        if (!keepImage) {
            images.remove(imagePath);
        }
        
        return instance.add(imagePath, temp);
    }
    catch (std::runtime_error& e) {
        utils::ConsoleCritical("VulkanTextures::createTextureHDR(): Failed to create a vulkan HDR texture: " + std::string(e.what()));
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

bool VulkanTextures::createDescriptorSetsLayout(uint32_t nBindlessTextures)
{
    VkDescriptorSetLayoutBinding bindlessSamplersLayoutBinding{};
    bindlessSamplersLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindlessSamplersLayoutBinding.descriptorCount = nBindlessTextures;
    bindlessSamplersLayoutBinding.binding = 0;
    bindlessSamplersLayoutBinding.pImmutableSamplers = nullptr;
    bindlessSamplersLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    VkDescriptorSetLayoutBinding bindlessStorageImagesLayoutBinding{};
    bindlessStorageImagesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindlessStorageImagesLayoutBinding.descriptorCount = nBindlessTextures;
    bindlessStorageImagesLayoutBinding.binding = 1;
    bindlessStorageImagesLayoutBinding.pImmutableSamplers = nullptr;
    bindlessStorageImagesLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    std::array<VkDescriptorSetLayoutBinding, 2> setBindings = { bindlessSamplersLayoutBinding, bindlessStorageImagesLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
    layoutInfo.pBindings = setBindings.data();
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;

    VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    std::array<VkDescriptorBindingFlags, 2> binding_flags = {bindlessFlags, bindlessFlags};
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo = {};
    extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
    extendedInfo.bindingCount = static_cast<uint32_t>(binding_flags.size());
    extendedInfo.pBindingFlags = binding_flags.data();
    extendedInfo.pNext = nullptr;

    layoutInfo.pNext = &extendedInfo;

    if (vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutTextures) != VK_SUCCESS) {
        utils::ConsoleCritical("VulkanTextures::createDescriptorSetsLayout(): Failed to create the material descriptor set layout");
        return false;
    }

    return true;
}

bool VulkanTextures::createDescriptorPool(uint32_t nImages, uint32_t nBindlessTextures)
{
    VkDescriptorPoolSize bindlessSamplersPoolSize{};
    bindlessSamplersPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindlessSamplersPoolSize.descriptorCount = nBindlessTextures;

    VkDescriptorPoolSize bindlessStorageImagesPoolSize{};
    bindlessStorageImagesPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindlessStorageImagesPoolSize.descriptorCount = nBindlessTextures;

    std::array<VkDescriptorPoolSize, 2> poolSizes = { bindlessSamplersPoolSize, bindlessStorageImagesPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
    
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    poolInfo.maxSets = static_cast<uint32_t>(nImages + nBindlessTextures * 2);

    if (vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("VulkanTextures::createDescriptorPool(): Failed to create descriptor pool");
        return false;
    }

    return true;
}

bool VulkanTextures::createDescriptorSets()
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_descriptorSetLayoutTextures;

    vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, &m_descriptorSetBindlessTextures);

    return true;
}