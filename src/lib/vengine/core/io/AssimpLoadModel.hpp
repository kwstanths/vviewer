#ifndef __AssimpLoadModel_hpp__
#define __AssimpLoadModel_hpp__

#include <string>
#include <memory>

#include <assimp/scene.h>

#include "ImportTypes.hpp"

namespace vengine
{

Tree<ImportedModelNode> assimpLoadModel(std::string filename);
Tree<ImportedModelNode> assimpLoadModel(std::string filename, std::vector<ImportedMaterial> &materials);

}  // namespace vengine

#endif