#include "VulkanRenderUtils.hpp"

namespace vengine
{

void parseSceneGraph(const SceneGraph &sceneObjectsArray,
                     std::vector<SceneObject *> &outMeshes,
                     std::vector<SceneObject *> &outLights,
                     std::vector<std::pair<SceneObject *, uint32_t>> &outMeshLights)
{
    uint32_t objectDescriptionIndex = 0;
    for (auto &itr : sceneObjectsArray) {
        /* Skip inactive objects */
        if (!itr->active()) {
            continue;
        }

        if (itr->has<ComponentMesh>() && itr->has<ComponentMaterial>()) {
            itr->computeAABB();

            outMeshes.push_back(itr);

            if (itr->get<ComponentMaterial>().material->isEmissive()) {
                outMeshLights.push_back({itr, objectDescriptionIndex});
            }

            objectDescriptionIndex++;
        }

        if (itr->has<ComponentLight>()) {
            outLights.push_back(itr);
        }
    }
}

}  // namespace vengine