#include "VulkanScene.hpp"

#include "core/AssetManager.hpp"

VulkanScene::VulkanScene()
{
}

VulkanScene::~VulkanScene()
{
}

void VulkanScene::setSkybox(VulkanMaterialSkybox* skybox)
{
    m_skybox = skybox;
}

VulkanMaterialSkybox* VulkanScene::getSkybox() const
{
    return m_skybox;
}

SceneData VulkanScene::getSceneData() const
{
    SceneData sceneData = Scene::getSceneData();
    sceneData.m_projection[1][1] *= -1;
    sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);

    return sceneData;
}

std::shared_ptr<SceneNode> VulkanScene::addSceneObject(std::string meshModel, Transform transform, std::string material)
{
    std::vector<std::shared_ptr<VulkanSceneObject>> objects = createObject(meshModel, material);
    
    std::shared_ptr<SceneNode> parentNode = std::make_shared<SceneNode>(std::make_shared<SceneObject>(nullptr), transform);
    m_sceneGraph.push_back(parentNode);
    for (auto& s : objects) {
        parentNode->addChild(s, Transform());
    }

    return parentNode;
}

std::shared_ptr<SceneNode> VulkanScene::addSceneObject(std::shared_ptr<SceneNode> node, std::string meshModel, Transform transform, std::string material)
{
    std::vector<std::shared_ptr<VulkanSceneObject>> objects = createObject(meshModel, material);

    std::shared_ptr<SceneNode> parentNode = node->addChild(std::make_shared<SceneObject>(nullptr), transform);
    for (auto& s : objects) {
        parentNode->addChild(s, Transform());
    }

    return parentNode;
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

std::vector<std::shared_ptr<VulkanSceneObject>> VulkanScene::createObject(std::string meshModel, std::string material)
{
    AssetManager<std::string, MeshModel*>& instanceModels = AssetManager<std::string, MeshModel*>::getInstance();
    if (!instanceModels.isPresent(meshModel)) return {};
    AssetManager<std::string, Material*>& instanceMaterials = AssetManager<std::string, Material*>::getInstance();
    if (!instanceMaterials.isPresent(material)) return {};

    MeshModel* vkmeshModel = instanceModels.Get(meshModel);
    std::vector<Mesh*> modelMeshes = vkmeshModel->getMeshes();
    
    std::vector<std::shared_ptr<VulkanSceneObject>> objects;
    for (auto& m : modelMeshes) {
        auto object = std::make_shared<VulkanSceneObject>(m, m_modelDataDynamicUBO, m_transformIndexUBO++);
        object->setMaterial(instanceMaterials.Get(material));
        object->m_name = m->m_name;
        objects.push_back(object);
    }

    return objects;
}
