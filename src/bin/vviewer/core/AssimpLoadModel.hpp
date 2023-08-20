#ifndef __AssimpLoadModel_hpp__
#define __AssimpLoadModel_hpp__

#include <string>
#include <assimp/scene.h>

#include "core/Mesh.hpp"

namespace vengine {

/**
 * @brief Load a model from the disk, and get back a list of meshes
 * 
 * @param filename Location in the disk for the model
 * @param applyTransformation If true, internal transformation will be directly applied on the input data
 * @return std::vector<Mesh> A list of meshes
 */
std::vector<Mesh> assimpLoadModel(std::string filename, bool applyTransformation = true);

glm::mat4 getTransformation(aiNode * node);

std::vector<Mesh> assimpLoadNode(aiNode * node, const aiScene * scene, bool applyTransformation = true, glm::mat4 parentTransformation = glm::mat4(1.0F));

Mesh assimpLoadMesh(aiMesh * mesh, const aiScene * scene, bool applyTransformation = true, glm::mat4 transfomation = glm::mat4(1.0F));

}

#endif