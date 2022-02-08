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

SceneObject* VulkanScene::addSceneObject(std::string meshModel, Transform transform, std::string material) 
{
    AssetManager<std::string, MeshModel*>& instanceModels = AssetManager<std::string, MeshModel*>::getInstance();
    if (!instanceModels.isPresent(meshModel)) return nullptr;
    AssetManager<std::string, Material*>& instanceMaterials = AssetManager<std::string, Material*>::getInstance();
    if (!instanceMaterials.isPresent(material)) return nullptr;

    MeshModel* vkmeshModel = instanceModels.Get(meshModel);

    VulkanSceneObject* object = new VulkanSceneObject(vkmeshModel, transform, m_modelDataDynamicUBO, m_transformIndexUBO++);
    m_objects.push_back(object);

    object->setMaterial(instanceMaterials.Get(material));

    return object;
}

std::vector<VulkanSceneObject*>& VulkanScene::getSceneObjects() 
{
    return m_objects;
}

void VulkanScene::updateBuffers(VkDevice device, uint32_t imageIndex) const
{
    /* Update scene data buffer */
    {
        SceneData sceneData;
        sceneData.m_view = m_camera->getViewMatrix();
        sceneData.m_projection = m_camera->getProjectionMatrix();
        sceneData.m_projection[1][1] *= -1;

        std::shared_ptr<DirectionalLight> light = getDirectionalLight();
        if (light != nullptr) {
            sceneData.m_directionalLightDir = glm::vec4(light->transform.getForward(), 0);
            sceneData.m_directionalLightColor = glm::vec4(light->color, 0);
        }
        else {
            sceneData.m_directionalLightColor = glm::vec4(0, 0, 0, 0);
        }

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
