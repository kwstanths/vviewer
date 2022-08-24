#include "Scene.hpp"

#include <algorithm>

#include "SceneExport.hpp"

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

void Scene::setSkybox(MaterialSkybox* skybox)
{
    m_skybox = skybox;
}

MaterialSkybox* Scene::getSkybox() const
{
    return m_skybox;
}

std::shared_ptr<SceneObject> Scene::addSceneObject(std::string meshModel, Transform transform, std::string material)
{
    std::vector<std::shared_ptr<SceneObject>> objects = createObject(meshModel, material);

    std::shared_ptr<SceneObject> parentObject = std::make_shared<SceneObject>(transform);
    m_sceneGraph.push_back(parentObject);
    for (auto& s : objects) {
        parentObject->addChild(s);
        m_objectsMap.insert({ s->getID(), s });
    }

    return parentObject;
}

std::shared_ptr<SceneObject> Scene::addSceneObject(std::shared_ptr<SceneObject> node, std::string meshModel, Transform transform, std::string material)
{
    std::vector<std::shared_ptr<SceneObject>> objects = createObject(meshModel, material);

    std::shared_ptr<SceneObject> parentObject = node->addChild(std::make_shared<SceneObject>(transform));
    for (auto& s : objects) {
        parentObject->addChild(s);
        m_objectsMap.insert({ s->getID(), s });
    }

    return parentObject;
}

void Scene::removeSceneObject(std::shared_ptr<SceneObject> node)
{
    /* TODO remove scene objects from m_objectsMap, resursively */

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
        temp.push_back(rootNode);
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
        temp.push_back(rootNode);
        modelMatrices.push_back(rootNode->m_modelMatrix);
        std::vector<glm::mat4> rootNodeMatrices;
        auto rootNodeObjects = rootNode->getSceneObjects(rootNodeMatrices);

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
        modelMatrices.insert(modelMatrices.end(), rootNodeMatrices.begin(), rootNodeMatrices.end());
    }

    return temp;
}

std::shared_ptr<SceneObject> Scene::getSceneObject(ID id) const
{
    auto itr = m_objectsMap.find(id);
    if (itr == m_objectsMap.end()) return std::shared_ptr<SceneObject>(nullptr);
    return itr->second;
}

void Scene::exportScene(std::string name, uint32_t width, uint32_t height, uint32_t samples) const
{
    exportJson(name, m_camera, m_sceneGraph, m_skybox->getMap(), width, height, samples);
}
