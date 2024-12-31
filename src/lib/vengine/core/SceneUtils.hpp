#ifndef __SceneUtils_hpp__
#define __SceneUtils_hpp__

#include "Scene.hpp"
#include "utils/ThreadPool.hpp"

namespace vengine
{

/* Update Scene objects */

void UpdateSceneObject(SceneObject *sceneObject);
void UpdateSceneGraph(SceneObjectVector &sceneGraph);

void UpdateSceneObjectParallel(SceneObject *sceneObject, ThreadPool& threadPool);
void UpdateSceneGraphParallel(SceneObjectVector &sceneGraph, ThreadPool &threadPool);

/**
* Adds a 3D model onto a scene
* @param scene The scene to add it in
* @param parent The parent scene object. Use nullptr for root
* @param modelName the 3D model name or asset name
* @param overrideRootTransform The Transform for the root node of the scene object
* @param overrideMaterial Override material to use
*/
void addModel3D(vengine::Scene &scene,
                vengine::SceneObject *parent,
                std::string modelName,
                std::optional<vengine::Transform> overrideRootTransform = std::nullopt,
                std::optional<std::string> overrideMaterial = std::nullopt);

}

#endif