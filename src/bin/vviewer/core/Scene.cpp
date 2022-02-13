#include "Scene.hpp"

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