#include "VulkanMaterials.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanMaterials::VulkanMaterials(VulkanContext &ctx)
    : m_vkctx(ctx)
    , m_materialsStorage(VULKAN_LIMITS_MAX_MATERIALS)
{
}

VkResult VulkanMaterials::initResources()
{
    VULKAN_CHECK_CRITICAL(m_materialsStorage.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment));

    if (m_materialsStorage.blockSizeAligned() != sizeof(MaterialData)) {
        std::string error =
            "VulkanMaterials::initResources(): The Material GPU block size is different than the size of MaterialData. GPU Block Size "
            "= " +
            std::to_string(m_materialsStorage.blockSizeAligned());
        debug_tools::ConsoleCritical(error);
        throw std::runtime_error(error);
    }

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout());

    return VK_SUCCESS;
}

VkResult VulkanMaterials::initSwapchainResources(uint32_t nImages)
{
    m_swapchainImages = nImages;

    VULKAN_CHECK_CRITICAL(m_materialsStorage.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages, 10));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));

    updateDescriptorSets();

    {
        AssetManager<std::string, MaterialSkybox> &instance = AssetManager<std::string, MaterialSkybox>::getInstance();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto material = std::static_pointer_cast<VulkanMaterialSkybox>(itr->second);
            VULKAN_CHECK_CRITICAL(material->createDescriptors(m_vkctx.device(), m_descriptorPool, m_swapchainImages));
            material->updateDescriptorSets(m_vkctx.device(), m_swapchainImages);
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanMaterials::releaseResources()
{
    /* Destroy material data */
    {
        AssetManager<std::string, Material> &instance = AssetManager<std::string, Material>::getInstance();
        instance.reset();
    }

    m_materialsStorage.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutMaterial, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanMaterials::releaseSwapchainResources()
{
    m_materialsStorage.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

void VulkanMaterials::updateBuffers(uint32_t index) const
{
    m_materialsStorage.updateBuffer(m_vkctx.device(), index);
}

VkBuffer VulkanMaterials::getBuffer(uint32_t index)
{
    return m_materialsStorage.buffer(index);
}

std::shared_ptr<Material> VulkanMaterials::createMaterial(std::string name, MaterialType type, bool createDescriptors)
{
    AssetManager<std::string, Material> &instance = AssetManager<std::string, Material>::getInstance();
    if (instance.isPresent(name)) {
        debug_tools::ConsoleWarning("Material name already present");
        return nullptr;
    }

    std::shared_ptr<Material> temp;
    switch (type) {
        case MaterialType::MATERIAL_PBR_STANDARD: {
            temp = std::make_shared<VulkanMaterialPBRStandard>(
                name, glm::vec4(1, 1, 1, 1), 0, 1, 1, 0, m_vkctx.device(), m_descriptorSetLayoutMaterial, m_materialsStorage);
            instance.add(name, temp);
            break;
        }
        case MaterialType::MATERIAL_LAMBERT: {
            temp = std::make_shared<VulkanMaterialLambert>(
                name, glm::vec4(1, 1, 1, 1), 1, 0, m_vkctx.device(), m_descriptorSetLayoutMaterial, m_materialsStorage);
            instance.add(name, temp);
            break;
        }
        default: {
            throw std::runtime_error("VulkanMaterialSystem::createMaterial(): Unexpected material");
            break;
        }
    }

    return temp;
}

VkResult VulkanMaterials::createDescriptorSetsLayout()
{
    /* Create binding for material data */
    VkDescriptorSetLayoutBinding materiaDatalLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 1);

    std::array<VkDescriptorSetLayoutBinding, 1> setBindings = {materiaDatalLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(setBindings.size()), setBindings.data());

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutMaterial));

    return VK_SUCCESS;
}

VkResult VulkanMaterials::createDescriptorPool(uint32_t nImages, uint32_t maxCubemaps)
{
    VkDescriptorPoolSize materialDataPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(nImages));
    VkDescriptorPoolSize cubemapDataPoolSize =
        vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(6 * maxCubemaps * nImages));
    std::array<VkDescriptorPoolSize, 2> poolSizes = {materialDataPoolSize, cubemapDataPoolSize};

    VkDescriptorPoolCreateInfo poolInfo = vkinit::descriptorPoolCreateInfo(
        static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), static_cast<uint32_t>(nImages + maxCubemaps * nImages));
    poolInfo.maxSets = static_cast<uint32_t>(nImages + maxCubemaps * nImages);

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanMaterials::createDescriptorSets(uint32_t nImages)
{
    m_descriptorSets.resize(nImages);

    std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutMaterial);
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSets.data()));

    return VK_SUCCESS;
}

void VulkanMaterials::updateDescriptor(uint32_t index)
{
    VkDescriptorBufferInfo bufferInfo = vkinit::descriptorBufferInfo(m_materialsStorage.buffer(index), 0, VK_WHOLE_SIZE);
    VkWriteDescriptorSet descriptorWrite =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfo);

    std::array<VkWriteDescriptorSet, 1> writeSets = {descriptorWrite};
    vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

void VulkanMaterials::updateDescriptorSets()
{
    for (uint32_t i = 0; i < m_descriptorSets.size(); i++) {
        updateDescriptor(i);
    }
}

}  // namespace vengine