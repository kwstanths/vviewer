#ifndef __Import_hpp__
#define __Import_hpp__

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include <rapidjson/document.h>

#include <glm/glm.hpp>

#include <utils/Tree.hpp>
#include <math/Transform.hpp>

#include "ImportTypes.hpp"

namespace vengine
{

std::string importScene(std::string filename,
                        ImportedCamera &c,
                        Tree<ImportedSceneObject> &sceneObjects,
                        std::vector<std::string> &models,
                        std::vector<ImportedMaterial> &materials,
                        std::vector<ImportedLightMaterial> &lightMaterials,
                        ImportedEnvironment &e);

}  // namespace vengine

#endif