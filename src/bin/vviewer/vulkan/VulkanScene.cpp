#include "VulkanScene.hpp"

VulkanScene::VulkanScene()
{
}

VulkanScene::~VulkanScene()
{
}

void VulkanScene::setCamera(std::shared_ptr<Camera> camera)
{
    m_camera = camera;
}

std::shared_ptr<Camera> VulkanScene::getCamera() const
{
    return m_camera;
}

void VulkanScene::setSkybox(VulkanMaterialSkybox* skybox)
{
    m_skybox = skybox;
}

VulkanMaterialSkybox* VulkanScene::getSkybox() const
{
    return m_skybox;
}

void VulkanScene::setDirectionalLight(std::shared_ptr<DirectionalLight> directionaLight)
{
    m_directionalLight = directionaLight;
}

std::shared_ptr<DirectionalLight> VulkanScene::getDirectionalLight() const
{
    return m_directionalLight;
}

VulkanSceneObject* VulkanScene::addObject(MeshModel* meshModel, Transform& transform)
{
    VulkanSceneObject* object = new VulkanSceneObject(meshModel, transform, m_modelDataDynamicUBO, m_transformIndexUBO++);
    m_objects.push_back(object);
    return object;
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
