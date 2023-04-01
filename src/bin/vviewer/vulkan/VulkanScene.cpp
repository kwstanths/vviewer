#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/VulkanLimits.hpp"

VulkanScene::VulkanScene(VulkanContext& vkctx, uint32_t maxObjects)
    : m_vkctx(vkctx), m_modelDataDynamicUBO(maxObjects)
{
}

VulkanScene::~VulkanScene()
{
}

bool VulkanScene::initResources()
{
    /* Initialize dynamic uniform buffers for model matrix data */
    m_modelDataDynamicUBO.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    bool ret = createDescriptorSetsLayouts();
    return ret;
}

bool VulkanScene::initSwapchainResources(uint32_t nImages)
{
    bool ret = createBuffers(nImages);
    ret = ret && createDescriptorPool(nImages);
    ret = ret && createDescriptorSets(nImages);
    return ret;
}

bool VulkanScene::releaseResources()
{
    m_modelDataDynamicUBO.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutScene, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutModel, nullptr);

    return true;
}

bool VulkanScene::releaseSwapchainResources()
{
    for(uint32_t i=0; i<m_uniformBuffersScene.size(); i++)
    {
        m_uniformBuffersScene[i].destroy(m_vkctx.device());
    }

    m_modelDataDynamicUBO.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return true;
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

        void* data;
        vkMapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].getvkmemory(), 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].getvkmemory());
    }

    /* Update transform data to buffer */
    {
        void* data;
        vkMapMemory(m_vkctx.device(), m_modelDataDynamicUBO.getBufferMemory(imageIndex), 0, m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks(), 0, &data);
        memcpy(data, m_modelDataDynamicUBO.getBlock(0), m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks());
        vkUnmapMemory(m_vkctx.device(), m_modelDataDynamicUBO.getBufferMemory(imageIndex));
    }
}

std::shared_ptr<SceneObject> VulkanScene::createObject(std::string name)
{
    auto object = std::make_shared<VulkanSceneObject>(m_modelDataDynamicUBO);
    object->m_name = name;
    return object;
}

bool VulkanScene::createDescriptorSetsLayouts()
{
    /* Create descriptor layout for the scene data */
    {
        VkDescriptorSetLayoutBinding sceneDataLayoutBinding{};
        sceneDataLayoutBinding.binding = 0;
        sceneDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
        sceneDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        sceneDataLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &sceneDataLayoutBinding;

        if (vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutScene) != VK_SUCCESS) {
            utils::ConsoleCritical("VulkanDescriptorSystem::createDescriptorSetsLayouts(): Failed to create a descriptor set layout for the scene data");
            return false;
        }
    }

    {
        /* Create descriptor layout for the model data */
        VkDescriptorSetLayoutBinding modelDataLayoutBinding{};
        modelDataLayoutBinding.binding = 0;
        modelDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelDataLayoutBinding.descriptorCount = 1;    /* If we have an array of uniform buffers */
        modelDataLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        modelDataLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &modelDataLayoutBinding;

        if (m_vkctx.deviceFunctions()->vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutModel) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to create a descriptor set layout for model data");
            return false;
        }
    }

    return true;
}

bool VulkanScene::createDescriptorPool(uint32_t nImages)
{
    VkDescriptorPoolSize sceneDataPoolSize{};
    sceneDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sceneDataPoolSize.descriptorCount = nImages;

    VkDescriptorPoolSize modelDataPoolSize{};
    modelDataPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelDataPoolSize.descriptorCount = nImages;

    std::array<VkDescriptorPoolSize, 2> poolSizes = { sceneDataPoolSize, modelDataPoolSize };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(2 * nImages + VULKAN_LIMITS_MAX_MATERIALS * nImages);

    if (m_vkctx.deviceFunctions()->vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a descriptor pool");
        return false;
    }

    return true;
}

bool VulkanScene::createDescriptorSets(uint32_t nImages)
{    
    /* Create descriptor sets */
    {
        m_descriptorSetsScene.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutScene);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_vkctx.deviceFunctions()->vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsScene.data()) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to allocate descriptor sets for the scene data");
            return false;
        }
    }

    {
        m_descriptorSetsModel.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutModel);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = nImages;
        allocInfo.pSetLayouts = layouts.data();
        if (m_vkctx.deviceFunctions()->vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsModel.data()) != VK_SUCCESS) {
            utils::ConsoleCritical("Failed to allocate descriptor sets for model data");
            return false;
        }
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {

        VkDescriptorBufferInfo bufferInfoScene{};
        bufferInfoScene.buffer = m_uniformBuffersScene[i].vkbuffer();
        bufferInfoScene.offset = 0;
        bufferInfoScene.range = sizeof(SceneData);
        VkWriteDescriptorSet descriptorWriteScene{};
        descriptorWriteScene.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteScene.dstSet = m_descriptorSetsScene[i];
        descriptorWriteScene.dstBinding = 0;
        descriptorWriteScene.dstArrayElement = 0;
        descriptorWriteScene.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteScene.descriptorCount = 1;
        descriptorWriteScene.pBufferInfo = &bufferInfoScene;
        descriptorWriteScene.pImageInfo = nullptr; // Optional
        descriptorWriteScene.pTexelBufferView = nullptr; // Optional
        
        VkDescriptorBufferInfo bufferInfoModel{};
        bufferInfoModel.buffer = m_modelDataDynamicUBO.getBuffer(i);
        bufferInfoModel.offset = 0;
        bufferInfoModel.range = m_modelDataDynamicUBO.getBlockSizeAligned();
        VkWriteDescriptorSet descriptorWriteModel{};
        descriptorWriteModel.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteModel.dstSet = m_descriptorSetsModel[i];
        descriptorWriteModel.dstBinding = 0;
        descriptorWriteModel.dstArrayElement = 0;
        descriptorWriteModel.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWriteModel.descriptorCount = 1;
        descriptorWriteModel.pBufferInfo = &bufferInfoModel;
        descriptorWriteModel.pImageInfo = nullptr; // Optional
        descriptorWriteModel.pTexelBufferView = nullptr; // Optional

        std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteScene, descriptorWriteModel };
        m_vkctx.deviceFunctions()->vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return true;
}

bool VulkanScene::createBuffers(uint32_t nImages)
{
    VkDeviceSize bufferSize = sizeof(SceneData);

    m_uniformBuffersScene.resize(nImages);
    for (int i = 0; i < nImages; i++) {
        createBuffer(m_vkctx.physicalDevice(), 
            m_vkctx.device(), 
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            m_uniformBuffersScene[i]);
    }

    m_modelDataDynamicUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    return true;
}