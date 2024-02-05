#ifndef __Import_hpp__
#define __Import_hpp__

#include <string>
#include <vector>

#include <utils/Tree.hpp>

#include "ImportTypes.hpp"

namespace vengine
{

std::string importScene(std::string filename,
                        ImportedCamera &c,
                        Tree<ImportedSceneObject> &sceneObjects,
                        std::vector<ImportedModel> &models,
                        std::vector<ImportedMaterial> &materials,
                        std::vector<ImportedLight> &lights,
                        ImportedEnvironment &e);

}  // namespace vengine

#endif