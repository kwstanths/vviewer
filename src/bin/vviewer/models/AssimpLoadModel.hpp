#ifndef __AssimpLoadModel_hpp__
#define __AssimpLoadModel_hpp__

#include <string>
#include <assimp/scene.h>

#include "core/Mesh.hpp"
#include "ImportedInfo.hpp"

std::vector<Mesh> assimpLoadModel(std::string filename);

std::vector<Mesh> assimpLoadNode(aiNode * node, const aiScene * scene);

Mesh assimpLoadMesh(aiMesh * mesh, const aiScene * scene);

#endif