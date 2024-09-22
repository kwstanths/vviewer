#include "SceneUtils.hpp"

namespace vengine
{

void UpdateSceneObject(SceneObject *sceneObject)
{
    sceneObject->update();

    for (SceneObject* child : sceneObject->children()) {
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

}