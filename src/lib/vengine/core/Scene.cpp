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
    lightMaterials.add(std::make_shared<LightMaterial>("DirectionalLightMaterial", glm::vec3(1, 0.9, 0.8), 0.F));
    lightMaterials.add(std::make_shared<LightMaterial>("DefaultPointLightMaterial", glm::vec3(1, 1, 1), 1.F));
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

std::shared_ptr<SceneObject> Scene::addSceneObject(std::string name, Transform transform)
{
    std::shared_ptr<SceneObject> object = createObject(name);
    object->localTransform() = transform;
    m_sceneGraph.push_back(object);

    m_objectsMap.insert({object->getID(), object});

    return object;
}

std::shared_ptr<SceneObject> Scene::addSceneObject(std::string name, std::shared_ptr<SceneObject> node, Transform transform)
{
    std::shared_ptr<SceneObject> object = createObject(name);
    object->localTransform() = transform;
    node->addChild(object);

    m_objectsMap.insert({object->getID(), object});

    return object;
}

void Scene::removeSceneObject(std::shared_ptr<SceneObject> node)
{
    /* Remove from scene graph */
    if (node->parent() == nullptr) {
        /* Remove scene node from root */
        m_sceneGraph.erase(std::remove(m_sceneGraph.begin(), m_sceneGraph.end(), node), m_sceneGraph.end());
    } else {
        /* Remove from parent */
        node->parent()->children().erase(std::remove(node->parent()->children().begin(), node->parent()->children().end(), node),
                                         node->parent()->children().end());
    }

    /* Remove from m_objectsMap recusrively */
    std::function<void(std::vector<std::shared_ptr<SceneObject>> &)> removeChildren;
    removeChildren = [&](std::vector<std::shared_ptr<SceneObject>> &children) {
        for (auto c : children) {
            m_objectsMap.erase(c->getID());
            removeChildren(c->children());
        }
    };
    m_objectsMap.erase(node->getID());
    removeChildren(node->children());
}

void Scene::updateSceneGraph()
{
    for (auto &node : m_sceneGraph) {
        node->update();
    }
}

std::vector<std::shared_ptr<SceneObject>> Scene::getSceneObjectsArray() const
{
    std::vector<std::shared_ptr<SceneObject>> temp;

    for (auto &&rootNode : m_sceneGraph) {
        temp.push_back(rootNode);
        auto rootNodeObjects = rootNode->getSceneObjectsArray();

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
    }

    return temp;
}

std::vector<std::shared_ptr<SceneObject>> Scene::getSceneObjectsArray(std::vector<glm::mat4> &modelMatrices) const
{
    std::vector<std::shared_ptr<SceneObject>> temp;
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

std::shared_ptr<SceneObject> Scene::getSceneObject(vengine::ID id) const
{
    auto itr = m_objectsMap.find(id);
    if (itr == m_objectsMap.end())
        return std::shared_ptr<SceneObject>(nullptr);
    return itr->second;
}

void Scene::exportScene(const ExportRenderParams &renderParams) const
{
    std::shared_ptr<EnvironmentMap> envMap = nullptr;
    if (m_skybox != nullptr && environmentType() == EnvironmentType::HDRI) {
        envMap = m_skybox->getMap();
    }

    exportJson(renderParams, m_camera, m_sceneGraph, envMap);
}

}  // namespace vengine
