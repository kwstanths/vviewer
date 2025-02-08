#include "SceneUtils.hpp"

#include "AssetManager.hpp"
#include <list>

namespace vengine
{

void UpdateSceneGraph(SceneObjectVector &sceneGraph)
{
    for (SceneObject *&node : sceneGraph) {
        node->update();
        UpdateSceneGraph(node->children());
    }
}

struct SceneObjectVectorUpdateTask : Task {
    SceneObjectVectorUpdateTask(SceneObjectVector &sov,
                                uint32_t s,
                                uint32_t e,
                                ThreadPool &tp,
                                SceneObjectVectorUpdateTask *parent = nullptr)
        : Task(parent)
        , sceneObjectVector(sov)
        , start(s)
        , end(e)
        , threadPool(tp){};

    ThreadPool &threadPool;
    SceneObjectVector &sceneObjectVector;
    uint32_t start, end;
    std::vector<SceneObjectVectorUpdateTask> childrenTasks;
    bool work(float &progress) override
    {
        for (uint32_t i = start; i < end; i++) {
            SceneObject *sceneObject = sceneObjectVector[i];
            sceneObject->update();
            createTasks(sceneObject->children(), childrenTasks, threadPool, this);
            for (Task &task : childrenTasks) {
                threadPool.push(&task);
            }
        }
        return true;
    }

    static void createTasks(SceneObjectVector &sos,
                            std::vector<SceneObjectVectorUpdateTask> &tasks,
                            ThreadPool &threadPool,
                            SceneObjectVectorUpdateTask *parent = nullptr)
    {
        uint32_t batchSize = static_cast<uint32_t>(sos.size()) / threadPool.threads();
        batchSize++;

        uint32_t start = 0;
        for (uint32_t start = 0; start < sos.size(); start += batchSize) {
            uint32_t s = std::min(start, static_cast<uint32_t>(sos.size() - 1));
            uint32_t e = std::min(start + batchSize, static_cast<uint32_t>(sos.size()));
            tasks.emplace_back(sos, s, e, threadPool, parent);
        }
    }
};

void UpdateSceneGraphParallel(SceneObjectVector &sceneGraph, ThreadPool &threadPool)
{
    std::vector<SceneObjectVectorUpdateTask> tasks;

    SceneObjectVectorUpdateTask::createTasks(sceneGraph, tasks, threadPool);
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

    std::function<void(const Tree<Model3D::Model3DNode> &, SceneObject *, bool)> addModelF =
        [&](const Tree<Model3D::Model3DNode> &nodeTree, SceneObject *parent, bool isRoot) {
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