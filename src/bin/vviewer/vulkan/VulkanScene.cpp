#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

VulkanScene::VulkanScene()
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

std::vector<std::shared_ptr<SceneObject>> VulkanScene::createObject(std::string meshModel, std::string material)
{
    AssetManager<std::string, MeshModel*>& instanceModels = AssetManager<std::string, MeshModel*>::getInstance();
    if (!instanceModels.isPresent(meshModel))
    {
        utils::ConsoleWarning("VulkanScene::CreateObject(): Mesh model " + meshModel + " is not imported");
        return {};
    }
    AssetManager<std::string, Material*>& instanceMaterials = AssetManager<std::string, Material*>::getInstance();
    if (!instanceMaterials.isPresent(material)) 
    {
        utils::ConsoleWarning("VulkanScene::CreateObject(): Material " + material + " is not created");
        return {};
    }

    MeshModel* vkmeshModel = instanceModels.Get(meshModel);
    std::vector<Mesh*> modelMeshes = vkmeshModel->getMeshes();
    
    std::vector<std::shared_ptr<SceneObject>> objects;
    for (auto& m : modelMeshes) {
        auto object = std::make_shared<VulkanSceneObject>(m, m_modelDataDynamicUBO, m_transformIndexUBO++);
        object->setMaterial(instanceMaterials.Get(material));
        object->m_name = m->m_name;
        objects.push_back(object);
    }

    return objects;
}
