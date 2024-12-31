#include "SceneUtils.hpp"

#include "AssetManager.hpp"
#include <list>

namespace vengine
{

void UpdateSceneObject(SceneObject *sceneObject)
{
    sceneObject->update();

    for (SceneObject *child : sceneObject->children()) {
        UpdateSceneObject(child);
    }
}

void UpdateSceneGraph(SceneObjectVector &sceneGraph)
{
    for (SceneObject *&node : sceneGraph) {
        UpdateSceneObject(node);
    }
}

struct SceneObjectVectorUpdateTask : Task {
    SceneObjectVectorUpdateTask(SceneObjectVector &sov, uint32_t s, uint32_t e, ThreadPool &tp)
        : sceneObjectVector(sov)
        , start(s)
        , end(e)
        , threadPool(tp){};

    ThreadPool &threadPool;
    SceneObjectVector &sceneObjectVector;
    uint32_t start, end;
    bool work(float &progress) override
    {
        for (uint32_t i = start; i < end; i++) {
            UpdateSceneObjectParallel(sceneObjectVector[i], threadPool);
        }
        return true;
    }
};

void UpdateSceneObjectParallel(SceneObject *sceneObject, ThreadPool &threadPool)
{
    sceneObject->update();

    SceneObjectVector &children = sceneObject->children();

    uint32_t totalThreads = threadPool.threads();
    uint32_t batchSize = static_cast<uint32_t>(children.size()) / totalThreads;
    batchSize++;

    std::list<SceneObjectVectorUpdateTask> tasks;
    uint32_t start = 0;
    for (uint32_t start = 0; start < children.size(); start += batchSize) {
        uint32_t s = std::min(start, static_cast<uint32_t>(children.size() - 1));
        uint32_t e = std::min(start + batchSize, static_cast<uint32_t>(children.size()));
        tasks.emplace_back(children, s, e, threadPool);
    }
    for (Task &task : tasks) {
        threadPool.push(&task);
    }
    for (Task &task : tasks) {
        task.wait();
    }
}

void UpdateSceneGraphParallel(SceneObjectVector &sceneGraph, ThreadPool &threadPool)
{
    std::vector<SceneObjectVectorUpdateTask> tasks;
    for (uint32_t i = 0; i < sceneGraph.size(); i++) {
        tasks.emplace_back(sceneGraph, i, i + 1, threadPool);
    }
    for (Task &task : tasks) {
        threadPool.push(&task);
    }
    for (Task &task : tasks) {
        task.wait();
    }
}

void addModel3D(vengine::Scene &scene,
                vengine::SceneObject *parent,
                std::string modelName,
                std::optional<vengine::Transform> overrideRootTransform,
                std::optional<std::string> overrideMaterial)
{
    auto &instanceModels = vengine::AssetManager::getInstance().modelsMap();

    auto model3D = instanceModels.get(modelName);
    assert(model3D != nullptr);

    const Tree<Model3D::Model3DNode> &modelNodeData = model3D->nodeTree();

    auto &instanceMaterials = vengine::AssetManager::getInstance().materialsMap();
    vengine::Material *defaultMat = instanceMaterials.get("defaultMaterial");
    vengine::Material *overrideMat = (overrideMaterial.has_value() ? instanceMaterials.get(overrideMaterial.value()) : nullptr);

    std::function<void(const Tree<Model3D::Model3DNode>&, SceneObject *, bool)> addModelF =
        [&](const Tree<Model3D::Model3DNode> &nodeTree, SceneObject *parent, bool isRoot) 
        {
            auto &modelNode = nodeTree.data();
            /* Add all meshes of current node under parent */
            for (uint32_t i = 0; i < modelNode.meshes.size(); i++) {
                auto &mesh = modelNode.meshes[i];
                auto mat = modelNode.materials[i];

                vengine::Transform nodeTransform = modelNode.transform;
                if (isRoot && overrideRootTransform.has_value()) {
                    nodeTransform = overrideRootTransform.value();
                }
                vengine::SceneObject *newObject = scene.addSceneObject(modelName, parent, nodeTransform);

                newObject->add<vengine::ComponentMesh>().setMesh(mesh);
                if (overrideMat != nullptr) {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(overrideMat);
                } else if (mat != nullptr) {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(mat);
                } else {
                    newObject->add<vengine::ComponentMaterial>().setMaterial(defaultMat);
                }
            }

            if (nodeTree.childrenCount() > 0) {
                vengine::Transform nodeTransform = modelNode.transform;
                if (isRoot && overrideRootTransform.has_value()) {
                    nodeTransform = overrideRootTransform.value();
                }
                vengine::SceneObject *newObject = scene.addSceneObject(modelNode.name, parent, nodeTransform);
                for (uint32_t i = 0; i < nodeTree.childrenCount(); i++) {
                    addModelF(nodeTree.child(i), newObject, false);
                }
            }
        };

    addModelF(modelNodeData, parent, true);
}

}  // namespace vengine