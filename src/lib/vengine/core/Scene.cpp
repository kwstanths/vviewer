#include "Scene.hpp"

#include <algorithm>
#include <functional>

#include "AssetManager.hpp"
#include "Console.hpp"
#include "core/Lights.hpp"
#include "core/SceneObject.hpp"
#include "math/Transform.hpp"
#include "utils/ECS.hpp"

namespace vengine
{

Scene::Scene()
{
    auto &lightMaterials = AssetManager::getInstance().lightMaterialsMap();
    lightMaterials.add(new LightMaterial("DirectionalLightMaterial", glm::vec3(1, 0.9, 0.8), 0.F));
    lightMaterials.add(new LightMaterial("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1.F));
}

Scene::~Scene()
{
}

SceneData Scene::getSceneData() const
{
    assert(m_camera != nullptr);

    SceneData sceneData;
    sceneData.m_view = m_camera->viewMatrix();
    sceneData.m_viewInverse = m_camera->viewMatrixInverse();
    sceneData.m_projection = m_camera->projectionMatrix();
    sceneData.m_projectionInverse = m_camera->projectionMatrixInverse();
    sceneData.m_exposure = glm::vec4(exposure(), ambientIBLFactor(), 0, 0);
    return sceneData;
}

SceneObject *Scene::addSceneObject(std::string name, Transform transform)
{
    SceneObject *object = createObject(name);
    object->localTransform() = transform;
    m_sceneGraph.push_back(object);

    m_objectsMap.insert({object->getID(), object});

    return object;
}

SceneObject *Scene::addSceneObject(std::string name, SceneObject *parentNode, Transform transform)
{
    SceneObject *object = createObject(name);
    object->localTransform() = transform;
    parentNode->addChild(object);

    m_objectsMap.insert({object->getID(), object});

    return object;
}

void Scene::removeSceneObject(SceneObject *node)
{
    /* Remove from scene graph */
    if (node->parent() == nullptr) {
        /* Remove scene node from root */
        m_sceneGraph.erase(std::remove(m_sceneGraph.begin(), m_sceneGraph.end(), node), m_sceneGraph.end());
    } else {
        /* Remove from parent */
        node->parent()->removeChild(node);
    }

    std::vector<SceneObject *> nodeChildren = node->children();
    ID nodeId = node->getID();

    m_objectsMap.erase(nodeId);
    deleteObject(node);

    /* Remove children recusrively */
    std::function<void(const std::vector<SceneObject *> &)> removeChildren;
    removeChildren = [&](const std::vector<SceneObject *> &children) {
        for (auto c : children) {
            std::vector<SceneObject *> cChildren = c->children();
            ID cId = c->getID();
            deleteObject(c);

            m_objectsMap.erase(cId);
            removeChildren(cChildren);
        }
    };

    removeChildren(nodeChildren);
}

void Scene::updateSceneGraph()
{
    for (auto &node : m_sceneGraph) {
        node->update();
    }
}

std::vector<SceneObject *> Scene::getSceneObjectsArray() const
{
    std::vector<SceneObject *> temp;

    for (auto &&rootNode : m_sceneGraph) {
        temp.push_back(rootNode);
        auto rootNodeObjects = rootNode->getSceneObjectsArray();

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
    }

    return temp;
}

std::vector<SceneObject *> Scene::getSceneObjectsArray(std::vector<glm::mat4> &modelMatrices) const
{
    std::vector<SceneObject *> temp;
    modelMatrices.clear();

    for (auto &&rootNode : m_sceneGraph) {
        temp.push_back(rootNode);
        modelMatrices.push_back(rootNode->modelMatrix());
        std::vector<glm::mat4> rootNodeMatrices;
        auto rootNodeObjects = rootNode->getSceneObjectsArray(rootNodeMatrices);

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
        modelMatrices.insert(modelMatrices.end(), rootNodeMatrices.begin(), rootNodeMatrices.end());
    }

    return temp;
}

SceneGraph &Scene::sceneGraph()
{
    return m_sceneGraph;
}

SceneObject *Scene::getSceneObject(vengine::ID id) const
{
    auto itr = m_objectsMap.find(id);
    if (itr == m_objectsMap.end())
        return nullptr;
    return itr->second;
}

void Scene::exportScene(const ExportRenderParams &renderParams) const
{
    EnvironmentMap *envMap = nullptr;
    if (m_skybox != nullptr && environmentType() == EnvironmentType::HDRI) {
        envMap = m_skybox->getMap();
    }

    exportJson(renderParams, m_camera, m_sceneGraph, envMap);
}

}  // namespace vengine