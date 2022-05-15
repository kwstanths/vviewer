#include "Scene.hpp"

#include <algorithm>

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::setCamera(std::shared_ptr<Camera> camera)
{
    m_camera = camera;
}

std::shared_ptr<Camera> Scene::getCamera() const
{
    return m_camera;
}

void Scene::setDirectionalLight(std::shared_ptr<DirectionalLight> directionaLight)
{
    m_directionalLight = directionaLight;
}

std::shared_ptr<DirectionalLight> Scene::getDirectionalLight() const
{
    return m_directionalLight;
}

float Scene::getExposure() const
{
    return m_exposure;
}

void Scene::setExposure(float exposure)
{
    m_exposure = exposure;
}

SceneData Scene::getSceneData() const
{
    SceneData sceneData;
    sceneData.m_view = m_camera->getViewMatrix();
    sceneData.m_viewInverse = m_camera->getViewMatrixInverse();
    sceneData.m_projection = m_camera->getProjectionMatrix();
    sceneData.m_projectionInverse = m_camera->getProjectionMatrixInverse();
    sceneData.m_exposure = glm::vec4(getExposure(), 0, 0, 0);

    std::shared_ptr<DirectionalLight> light = getDirectionalLight();
    if (light != nullptr) {
        sceneData.m_directionalLightDir = glm::vec4(light->transform.getForward(), 0);
        sceneData.m_directionalLightColor = glm::vec4(light->color, 0);
    }
    else {
        sceneData.m_directionalLightColor = glm::vec4(0, 0, 0, 0);
    }

    return sceneData;
}

void Scene::removeSceneObject(std::shared_ptr<SceneNode> node)
{
    if (node->m_parent == nullptr) {
        /* Remove scene node from root */
        m_sceneGraph.erase(std::remove(m_sceneGraph.begin(), m_sceneGraph.end(), node), m_sceneGraph.end());
    }
    else {
        /* Remove from parent */
        node->m_parent->m_children.erase(
            std::remove(node->m_parent->m_children.begin(), node->m_parent->m_children.end(), node), 
            node->m_parent->m_children.end()
        );
    }
}

void Scene::updateSceneGraph()
{
    for (auto& node : m_sceneGraph) {
        node->update();
    }
}

std::vector<std::shared_ptr<SceneObject>> Scene::getSceneObjects() const
{
    std::vector<std::shared_ptr<SceneObject>> temp;

    for (auto&& rootNode : m_sceneGraph)
    {
        auto rootNodeObjects = rootNode->getSceneObjects();
        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
    }

    return temp;
}

std::vector<std::shared_ptr<SceneObject>> Scene::getSceneObjects(std::vector<glm::mat4>& modelMatrices) const
{
    std::vector<std::shared_ptr<SceneObject>> temp;
    modelMatrices.clear();

    for (auto&& rootNode : m_sceneGraph)
    {
        std::vector<glm::mat4> rootNodeMatrices;
        auto rootNodeObjects = rootNode->getSceneObjects(rootNodeMatrices);

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
        modelMatrices.insert(modelMatrices.end(), rootNodeMatrices.begin(), rootNodeMatrices.end());
    }

    return temp;
}
