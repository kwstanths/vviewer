#ifndef __SceneUtils_hpp__
#define __SceneUtils_hpp__

#include "Scene.hpp"
#include "utils/ThreadPool.hpp"

namespace vengine
{

void UpdateSceneObject(SceneObject *sceneObject);
void UpdateSceneGraph(SceneObjectVector &sceneGraph);

void UpdateSceneObjectParallel(SceneObject *sceneObject, ThreadPool& threadPool);
void UpdateSceneGraphParallel(SceneObjectVector &sceneGraph, ThreadPool &threadPool);

}

#endif