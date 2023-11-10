#ifndef __Export_hpp__
#define __Export_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include <vengine/core/SceneObject.hpp>
#include <vengine/core/Camera.hpp>
#include <vengine/core/Material.hpp>
#include <vengine/core/Light.hpp>

namespace vengine
{

struct ExportRenderParams {
    std::string name;

    int environmentType;
    glm::vec3 backgroundColor;
};

void exportJson(const ExportRenderParams &renderParams,
                std::shared_ptr<Camera> sceneCamera,
                const SceneGraph &sceneObjects,
                EnvironmentMap *envMap);

}  // namespace vengine

#endif