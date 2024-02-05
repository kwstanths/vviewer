#ifndef __VulkanRenderUtils_hpp__
#define __VulkanRenderUtils_hpp__

#include "vengine/core/SceneObject.hpp"

namespace vengine
{

void parseSceneGraph(const SceneGraph &sceneObjectsArray,
                     std::vector<SceneObject *> &outMeshes,
                     std::vector<SceneObject *> &outLights,
                     std::vector<std::pair<SceneObject *, uint32_t>> &outMeshLights);

}  // namespace vengine

#endif