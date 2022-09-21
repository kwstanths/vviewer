#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

VulkanScene::VulkanScene(uint32_t maxObjects)
    : m_modelDataDynamicUBO(maxObjects)
{
}

VulkanScene::~VulkanScene()
{
}

SceneData VulkanScene::getSceneData() const
{
    SceneData sceneData = Scene::getSceneData();
    sceneData.m_projection[1][1] *= -1;
    sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);

    return sceneData;
}

void VulkanScene::updateBuffers(VkDevice device, uint32_t imageIndex) const
{
    /* Update scene data buffer */
    {
        SceneData sceneData = VulkanScene::getSceneData();

        void* data;
        vkMapMemory(device, m_uniformBuffersSceneMemory[imageIndex], 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(device, m_uniformBuffersSceneMemory[imageIndex]);
    }

    /* Update transform data changes to buffer */
    {
        void* data;
        vkMapMemory(device, m_modelDataDynamicUBO.getBufferMemory(imageIndex), 0, m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks(), 0, &data);
        memcpy(data, m_modelDataDynamicUBO.getBlock(0), m_modelDataDynamicUBO.getBlockSizeAligned() * m_modelDataDynamicUBO.getNBlocks());
        vkUnmapMemory(device, m_modelDataDynamicUBO.getBufferMemory(imageIndex));
    }
}

std::shared_ptr<SceneObject> VulkanScene::createObject(std::string name)
{
    auto object = std::make_shared<VulkanSceneObject>(m_modelDataDynamicUBO);
    object->m_name = name;
    return object;
}
