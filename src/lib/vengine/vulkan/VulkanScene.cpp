#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"

namespace vengine
{

VulkanScene::VulkanScene(VulkanContext &vkctx, uint32_t maxObjects)
    : m_vkctx(vkctx)
    , m_modelDataDynamicUBO(maxObjects)
{
}

VulkanScene::~VulkanScene()
{
}

VkResult VulkanScene::initResources()
{
    /* Initialize dynamic uniform buffers for model matrix data */
    m_modelDataDynamicUBO.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayouts());

    return VK_SUCCESS;
}

VkResult VulkanScene::initSwapchainResources(uint32_t nImages)
{
    VULKAN_CHECK_CRITICAL(createBuffers(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));
    return VK_SUCCESS;
}

VkResult VulkanScene::releaseResources()
{
    m_modelDataDynamicUBO.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutScene, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutModel, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanScene::releaseSwapchainResources()
{
    for (uint32_t i = 0; i < m_uniformBuffersScene.size(); i++) {
        m_uniformBuffersScene[i].destroy(m_vkctx.device());
    }

    m_modelDataDynamicUBO.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

SceneData VulkanScene::getSceneData() const
{
    SceneData sceneData = Scene::getSceneData();
    sceneData.m_projection[1][1] *= -1;
    sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);

    return sceneData;
}

void VulkanScene::updateBuffers(uint32_t imageIndex) const
{
    /* Update scene data buffer */
    {
        SceneData sceneData = VulkanScene::getSceneData();

        void *data;
        vkMapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory(), 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory());
    }

    /* Update transform data to buffer */
    {
        void *data;
        vkMapMemory(m_vkctx.device(),
                    m_modelDataDynamicUBO.memory(imageIndex),
                    0,
                    m_modelDataDynamicUBO.blockSizeAligned() * m_modelDataDynamicUBO.nblocks(),
                    0,
                    &data);
        memcpy(data, m_modelDataDynamicUBO.block(0), m_modelDataDynamicUBO.blockSizeAligned() * m_modelDataDynamicUBO.nblocks());
        vkUnmapMemory(m_vkctx.device(), m_modelDataDynamicUBO.memory(imageIndex));
    }
}

std::shared_ptr<SceneObject> VulkanScene::createObject(std::string name)
{
    auto object = std::make_shared<VulkanSceneObject>(m_modelDataDynamicUBO);
    object->m_name = name;
    return object;
}

VkResult VulkanScene::createDescriptorSetsLayouts()
{
    /* Create descriptor layout for the scene data */
    {
        VkDescriptorSetLayoutBinding sceneDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &sceneDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutScene));
    }

    {
        /* Create descriptor layout for the model data */
        VkDescriptorSetLayoutBinding modelDataLayoutBinding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &modelDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutModel));
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createDescriptorPool(uint32_t nImages)
{
    VkDescriptorPoolSize sceneDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize modelDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, nImages);
    std::array<VkDescriptorPoolSize, 2> poolSizes = {sceneDataPoolSize, modelDataPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),
                                         poolSizes.data(),
                                         static_cast<uint32_t>(2 * nImages + VULKAN_LIMITS_MAX_MATERIALS * nImages));
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanScene::createDescriptorSets(uint32_t nImages)
{
    /* Create descriptor sets */
    {
        m_descriptorSetsScene.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutScene);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsScene.data()));
    }

    {
        m_descriptorSetsModel.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutModel);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsModel.data()));
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {
        auto bufferInfoScene = vkinit::descriptorBufferInfo(m_uniformBuffersScene[i].buffer(), 0, sizeof(SceneData));
        auto descriptorWriteScene =
            vkinit::writeDescriptorSet(m_descriptorSetsScene[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoScene);

        auto bufferInfoModel =
            vkinit::descriptorBufferInfo(m_modelDataDynamicUBO.buffer(i), 0, m_modelDataDynamicUBO.blockSizeAligned());
        auto descriptorWriteModel =
            vkinit::writeDescriptorSet(m_descriptorSetsModel[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0, 1, &bufferInfoModel);

        std::array<VkWriteDescriptorSet, 2> writeSets = {descriptorWriteScene, descriptorWriteModel};
        vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createBuffers(uint32_t nImages)
{
    VkDeviceSize bufferSize = sizeof(SceneData);

    m_uniformBuffersScene.resize(nImages);
    for (int i = 0; i < nImages; i++) {
        VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                           m_vkctx.device(),
                                           bufferSize,
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           m_uniformBuffersScene[i]));
    }

    m_modelDataDynamicUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    return VK_SUCCESS;
}

}  // namespace vengine