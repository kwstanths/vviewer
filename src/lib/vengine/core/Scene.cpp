#include "Scene.hpp"

#include <algorithm>
#include <functional>

#include "Engine.hpp"
#include "AssetManager.hpp"
#include "core/Light.hpp"
#include "core/SceneObject.hpp"
#include "core/SceneUtils.hpp"
#include "math/Transform.hpp"
#include "utils/ECS.hpp"

#include "debug_tools/Console.hpp"
#include "debug_tools/Timer.hpp"

//#define PRINT_UPDATE_TIME

namespace vengine
{

Scene::Scene(Engine &engine)
    : m_engine(engine)
{
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
    sceneData.m_exposure = glm::vec4(exposure(), environmentIntensity(), m_camera->lensRadius(), m_camera->focalDistance());
    sceneData.m_background = glm::vec4(m_backgroundColor, static_cast<float>(m_environmentType));
    sceneData.m_volumes = glm::vec4(-1, m_camera->znear(), m_camera->zfar(), 0);
    return sceneData;
}

SceneObject *Scene::addSceneObject(std::string name, Transform transform)
{
    return addSceneObject(name, nullptr, transform);
}

SceneObject *Scene::addSceneObject(std::string name, SceneObject *parentNode, Transform transform)
{
    SceneObject *object = createObject(name);
    object->setLocalTransform(transform);
    if (parentNode == nullptr) {
        m_sceneGraph.push_back(object);
    } else {
        parentNode->addChild(object);
    }

    m_objectsMap.insert({object->getID(), object});

    m_sceneGraphNeedsUpdate = true;
    m_instancesNeedUpdate = true;

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

    SceneObjectVector nodeChildren = node->children();
    ID nodeId = node->getID();

    m_objectsMap.erase(nodeId);
    deleteObject(node);

    /* Remove children recusrively */
    std::function<void(const SceneObjectVector &)> removeChildren;
    removeChildren = [&](const SceneObjectVector &children) {
        for (auto c : children) {
            SceneObjectVector cChildren = c->children();
            ID cId = c->getID();
            deleteObject(c);

            m_objectsMap.erase(cId);
            removeChildren(cChildren);
        }
    };

    removeChildren(nodeChildren);

    m_sceneGraphNeedsUpdate = true;
    m_instancesNeedUpdate = true;
}

void Scene::update()
{
#ifdef PRINT_UPDATE_TIME
    debug_tools::Timer timer;
    timer.Start();
#endif

    bool tlasNeedsUpdate = m_sceneGraphNeedsUpdate || m_instancesNeedUpdate;
    if (tlasNeedsUpdate) {
        invalidateTLAS();
    }

    if (m_sceneGraphNeedsUpdate) {
        m_sceneGraphNeedsUpdate = false;

        UpdateSceneGraphParallel(m_sceneGraph, m_engine.threadPool());
    }

#ifdef PRINT_UPDATE_TIME
    timer.Stop();
    if (timer.ToInt() != 0) {
        debug_tools::ConsoleInfo("Scene update time: " + timer.ToString() + " ms");
    }

    timer.Start();
#endif

    if (m_instancesNeedUpdate) {
        m_instancesNeedUpdate = false;
        instancesManager().invalidate();
    }

    instancesManager().build();

#ifdef PRINT_UPDATE_TIME
    timer.Stop();
    if (timer.ToInt() != 0) {
        debug_tools::ConsoleInfo("Instances build time: " + timer.ToString() + " ms");
    }
#endif
}

SceneObjectVector Scene::getSceneObjectsFlat() const
{
    SceneObjectVector temp;

    for (auto &&rootNode : m_sceneGraph) {
        temp.push_back(rootNode);
        auto rootNodeObjects = rootNode->getSceneNodesFlat();

        temp.insert(temp.end(), rootNodeObjects.begin(), rootNodeObjects.end());
    }

    return temp;
}

SceneObjectVector &Scene::sceneGraph()
{
    return m_sceneGraph;
}

SceneObject *Scene::findSceneObjectByID(vengine::ID id) const
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

void Scene::clear()
{
    std::function<void(const SceneObjectVector &)> deleteChildren;
    deleteChildren = [&](const SceneObjectVector &children) {
        for (auto c : children) {
            SceneObjectVector cChildren = c->children();
            deleteObject(c);
            deleteChildren(cChildren);
        }
    };

    for (SceneObject *so : m_sceneGraph) {
        SceneObjectVector nodeChildren = so->children();
        deleteObject(so);
        /* Delete children recusrively */
        deleteChildren(nodeChildren);
    }
    m_sceneGraph.clear();
    m_objectsMap.clear();
    m_sceneGraphNeedsUpdate = true;
    m_instancesNeedUpdate = true;
}

void Scene::invalidateSceneGraph(bool changed)
{
    m_sceneGraphNeedsUpdate = changed;
}

void Scene::invalidateInstances(bool changed)
{
    m_instancesNeedUpdate = changed;
}

}  // namespace vengine
