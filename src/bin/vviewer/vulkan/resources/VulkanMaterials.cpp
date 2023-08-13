#include "VulkanMaterials.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/VulkanLimits.hpp"

VulkanMaterials::VulkanMaterials(VulkanContext& ctx) : m_vkctx(ctx), m_materialsStorage(VULKAN_LIMITS_MAX_MATERIALS)
{
}

bool VulkanMaterials::initResources()
{
    bool ret = m_materialsStorage.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    if (m_materialsStorage.getBlockSizeAligned() != sizeof(MaterialData))
    {
        std::string error = "VulkanMaterials::initResources(): The Material GPU block size is different than the size of MaterialData. GPU Block Size = " + m_materialsStorage.getBlockSizeAligned();
        utils::ConsoleCritical(error);
        throw std::runtime_error(error);
    }

    ret = ret && createDescriptorSetsLayout();
    return ret;
}

bool VulkanMaterials::initSwapchainResources(uint32_t nImages)
{
    m_swapchainImages = nImages;

    bool ret = m_materialsStorage.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);
    ret = ret && createDescriptorPool(nImages, 10);
    ret = ret && createDescriptorSets(nImages);

    updateDescriptorSets();

    {
        AssetManager<std::string, MaterialSkybox>& instance = AssetManager<std::string, MaterialSkybox>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto material = std::static_pointer_cast<VulkanMaterialSkybox>(itr->second);
            material->createDescriptors(m_vkctx.device(), m_descriptorPool, m_swapchainImages);
            material->updateDescriptorSets(m_vkctx.device(), m_swapchainImages);
        }
    }


    return ret;
}

bool VulkanMaterials::releaseResources()
{
    /* Destroy material data */
    {
        AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
        instance.reset();
    }

    m_materialsStorage.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutMaterial, nullptr);

    return true;
}

bool VulkanMaterials::releaseSwapchainResources()
{
    m_materialsStorage.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return true;
}

void VulkanMaterials::updateBuffers(uint32_t index) const
{
    m_materialsStorage.updateBuffer(m_vkctx.device(), index);
}

VkBuffer VulkanMaterials::getBuffer(uint32_t index)
{
    return m_materialsStorage.getBuffer(index);
}

std::shared_ptr<Material> VulkanMaterials::createMaterial(std::string name, MaterialType type, bool createDescriptors)
{
    AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
    if (instance.isPresent(name)) {
        utils::ConsoleWarning("Material name already present");
        return nullptr;
    }

    std::shared_ptr<Material> temp;
    switch (type)
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
    {
        temp = std::make_shared<VulkanMaterialPBRStandard>(name, glm::vec4(1, 1, 1, 1), 0, 1, 1, 0, m_vkctx.device(), m_descriptorSetLayoutMaterial, m_materialsStorage);
        instance.add(name, temp);
        break;
    }
    case MaterialType::MATERIAL_LAMBERT:
    {
        temp = std::make_shared<VulkanMaterialLambert>(name, glm::vec4(1, 1, 1, 1), 1, 0, m_vkctx.device(), m_descriptorSetLayoutMaterial, m_materialsStorage);
        instance.add(name, temp);
        break;
    }
    default:
    {
        throw std::runtime_error("VulkanMaterialSystem::createMaterial(): Unexpected material");
        break;
    }
    }

    return temp;
}

bool VulkanMaterials::createDescriptorSetsLayout()
{
    {
        /* Create binding for material data */
        VkDescriptorSetLayoutBinding materiaDatalLayoutBinding{};
        materiaDatalLayoutBinding.binding = 0;
        materiaDatalLayoutBinding.descriptorCount = 1;
        materiaDatalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materiaDatalLayoutBinding.pImmutableSamplers = nullptr;
        materiaDatalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        std::array<VkDescriptorSetLayoutBinding, 1> setBindings = { materiaDatalLayoutBinding };
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
        layoutInfo.pBindings = setBindings.data();

        if (vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutMaterial) != VK_SUCCESS) {
            utils::ConsoleCritical("VulkanMaterialSystem::createDescriptorSetsLayout(): Failed to create the material descriptor set layout");
            return false;
        }
    }

    return true;
}

bool VulkanMaterials::createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps)
{
    VkDescriptorPoolSize materialDataPoolSize{};
    materialDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialDataPoolSize.descriptorCount = static_cast<uint32_t>(nImages);

    VkDescriptorPoolSize cubemapDataPoolSize{};
    cubemapDataPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubemapDataPoolSize.descriptorCount = static_cast<uint32_t>(6 * maxCubemaps * nImages);

    std::array<VkDescriptorPoolSize, 2> poolSizes = { materialDataPoolSize, cubemapDataPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;    
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(nImages + maxCubemaps * nImages);

    if (vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create material a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanMaterials::createDescriptorSets(uint32_t nImages)
{
    m_descriptorSets.resize(nImages);

    std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutMaterial);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = nImages;
    allocInfo.pSetLayouts = layouts.data();

    vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSets.data());

    return true;
}

void VulkanMaterials::updateDescriptor(uint32_t index)
{
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = m_materialsStorage.getBuffer(index);
    bufferInfo.offset = 0;
    bufferInfo.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_descriptorSets[index];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr; // Optional
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    std::array<VkWriteDescriptorSet, 1> writeSets = { descriptorWrite };
    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);  
}

bool VulkanMaterials::updateDescriptorSets()
{
    for(uint32_t i = 0; i < m_descriptorSets.size(); i++)
    {
        updateDescriptor(i);
    }

    return true;
}