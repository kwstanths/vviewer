#ifndef __Export_hpp__
#define __Export_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include <core/Camera.hpp>
#include <core/SceneObject.hpp>
#include <core/Material.hpp>
#include <core/Lights.hpp>

namespace vengine
{

struct ExportRenderParams {
    std::string name;

    int environmentType;
    glm::vec3 backgroundColor;
};

void exportJson(const ExportRenderParams &renderParams,
                std::shared_ptr<Camera> sceneCamera,
                const std::vector<std::shared_ptr<SceneObject>> &sceneObjects,
                std::shared_ptr<EnvironmentMap> envMap);

}  // namespace vengine

#endif