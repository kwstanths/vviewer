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

std::vector<std::shared_ptr<SceneObject>> Scene::getSceneObjects()
{
    std::vector<std::shared_ptr<SceneObject>> temp;

    for (auto&& rootNode : m_sceneGraph)
    {
        auto rootNodeObjects = rootNode->getSceneObjects();
        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
    }

    return temp;
}