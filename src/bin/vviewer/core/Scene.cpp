#include "Scene.hpp"

#include <algorithm>
#include <functional>

#include "AssetManager.hpp"

Scene::Scene()
{
    LightMaterial * directionalLightMaterial = new LightMaterial("DirectionalLightMaterial", glm::vec3(1, 0.9, 0.8), 0.F);
    LightMaterial * defaultPointLightMaterial = new LightMaterial("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1.F);
    
    Transform directionalLightTransform;
    directionalLightTransform.setRotationEuler(0, glm::radians(180.0f), 0);
    m_directionalLight = std::make_shared<DirectionalLight>(directionalLightTransform, directionalLightMaterial);
    
    auto& lightMaterials = AssetManager<std::string, LightMaterial *>::getInstance();
    lightMaterials.add(directionalLightMaterial->name, directionalLightMaterial);
    lightMaterials.add(defaultPointLightMaterial->name, defaultPointLightMaterial);
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

float Scene::getAmbientIBL() const
{
    return m_ambientIBL;
}

void Scene::setAmbientIBL(float ambientIBL)
{
    m_ambientIBL = ambientIBL;
}

SceneData Scene::getSceneData() const
{
    SceneData sceneData;
    sceneData.m_view = m_camera->getViewMatrix();
    sceneData.m_viewInverse = m_camera->getViewMatrixInverse();
    sceneData.m_projection = m_camera->getProjectionMatrix();
    sceneData.m_projectionInverse = m_camera->getProjectionMatrixInverse();
    sceneData.m_exposure = glm::vec4(getExposure(), getAmbientIBL(), 0, 0);

    std::shared_ptr<DirectionalLight> light = getDirectionalLight();
    if (light != nullptr) {
        sceneData.m_directionalLightDir = glm::vec4(light->transform.getForward(), 0);
        sceneData.m_directionalLightColor = light->lightMaterial->intensity * glm::vec4(light-> lightMaterial->color, 0);
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

EnvironmentType Scene::getEnvironmentType() const
{
    return m_environmentType;
}

void Scene::setEnvironmentType(EnvironmentType type)
{
    m_environmentType = type;
}

glm::vec3 Scene::getBackgroundColor() const
{
    return m_backgroundColor;
}

void Scene::setBackgroundColor(glm::vec3 color)
{
    m_backgroundColor = color;
}

std::shared_ptr<SceneObject> Scene::addSceneObject(std::string name, Transform transform)
{
    std::shared_ptr<SceneObject> object = createObject(name);
    object->m_localTransform = transform;
    m_sceneGraph.push_back(object);
    
    m_objectsMap.insert({ object->getID(), object });
    
    return object;
}

std::shared_ptr<SceneObject> Scene::addSceneObject(std::string name, std::shared_ptr<SceneObject> node, Transform transform)
{
    std::shared_ptr<SceneObject> object = createObject(name);
    object->m_localTransform = transform;
    node->addChild(object);
    
    m_objectsMap.insert({ object->getID(), object });
    
    return object;
}

void Scene::removeSceneObject(std::shared_ptr<SceneObject> node)
{
    /* Remove from scene graph */
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

    /* Remove from m_objectsMap recusrively */
    std::function<void(std::vector<std::shared_ptr<SceneObject>>&)> removeChildren;
    removeChildren = [&](std::vector<std::shared_ptr<SceneObject>>& children) { 
        for(auto c : children){
            m_objectsMap.erase(c->getID());
            removeChildren(c->m_children);
        }    
    };
    m_objectsMap.erase(node->getID());
    removeChildren(node->m_children);
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

void Scene::exportScene(const ExportRenderParams& renderParams) const
{
    EnvironmentMap* temp = nullptr;
    if (m_skybox != nullptr && getEnvironmentType() == EnvironmentType::HDRI)
    {
        temp = m_skybox->getMap();
    }

    exportJson(renderParams, m_camera, m_directionalLight, m_sceneGraph, temp);
}
