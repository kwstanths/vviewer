#include "AssimpLoadModel.hpp"

#include <stdexcept>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

#include "utils/Console.hpp"

#include "math/Transform.hpp"

#define SANITIZATION_CHECK

std::vector<Mesh> assimpLoadModel(std::string filename)
{
    Assimp::Importer importer;

    const aiScene * scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene) {
        importer.FreeScene();
        throw std::runtime_error("Failed to load model: " + filename);
    }

    std::vector<Mesh> temp = assimpLoadNode(scene->mRootNode, scene);
    importer.FreeScene();
    return temp;
}

std::vector<Mesh> assimpLoadNode(aiNode * node, const aiScene * scene)
{
    std::vector<Mesh> meshList;

    /* Loop through all meshes in this node */
    for (size_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh * mesh = scene->mMeshes[node->mMeshes[i]];
        meshList.push_back(assimpLoadMesh(mesh, scene));
    }

    /* Loop through all nodes attached to this node, and append their meshes */
    for (size_t i = 0; i < node->mNumChildren; i++) {
        std::vector<Mesh> childrenMeshes = assimpLoadNode(node->mChildren[i], scene);
        meshList.insert(meshList.end(), childrenMeshes.begin(), childrenMeshes.end());
    }

    return meshList;
}

bool checkValid(glm::vec3 in) {
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y) && !std::isnan(in.z) && !std::isinf(in.z);
}

bool checkValid(glm::vec2 in) {
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y);
}

Mesh assimpLoadMesh(aiMesh * mesh, const aiScene * scene)
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    bool hasNormals = mesh->HasNormals();
    bool hasUVs = mesh->HasTextureCoords(0);
    bool hasTangents = mesh->HasTangentsAndBitangents();

    auto meshName = std::string(mesh->mName.C_Str());

    bool printErrorMessage = true;
    vertices.resize(mesh->mNumVertices);
    for (size_t i = 0; i < mesh->mNumVertices; i++) {
        vertices[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        #ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].position)) utils::ConsoleCritical("assimpLoadMesh(): Input vertex for mesh " + meshName + " is corrupted");
        #endif

        if (hasNormals) {
            vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

            #ifdef SANITIZATION_CHECK
                if (!checkValid(vertices[i].normal)) utils::ConsoleCritical("assimpLoadMesh(): Input normal for mesh " + meshName + " is corrupted");
                if (glm::length(vertices[i].normal) < 0.99) {
                    utils::ConsoleCritical("assimpLoadMesh(): Input normal for mesh " + meshName + " is not normalized");
                    vertices[i].normal = glm::normalize(vertices[i].normal);
                }
            #endif
        }

        if (mesh->mTextureCoords[0]) {
            /* Flip v coordinate vertically because of vulkan */
            vertices[i].uv = { mesh->mTextureCoords[0][i].x, 1.f - mesh->mTextureCoords[0][i].y };

            #ifdef SANITIZATION_CHECK
                if (!checkValid(vertices[i].uv)) utils::ConsoleCritical("assimpLoadMesh(): Input uv for mesh " + meshName + " is corrupted");
            #endif

        } else {
            vertices[i].uv = { 0, 0 };
        }

        vertices[i].color = { 1, 1, 1 };

        if (hasTangents) {
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertices[i].bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

            #ifdef SANITIZATION_CHECK
                if (!checkValid(vertices[i].tangent) || !checkValid(vertices[i].bitangent))
                {
                    if (printErrorMessage) {
                        utils::ConsoleWarning("assimpLoadMesh(): Input tangent or bitangent for mesh " + meshName + " is corrupted. Attempting reconstruction from normal...");
                        printErrorMessage = false;
                    }

                    auto t1 = glm::cross(vertices[i].normal, Transform::Z);
                    auto t2 = glm::cross(vertices[i].normal, Transform::X);
                    if(glm::length(t1) > glm::length(t2)) {
                        vertices[i].tangent = t1;
                    }
                    else {
                        vertices[i].tangent = t2;
                    }

                    vertices[i].bitangent = glm::cross(vertices[i].normal, vertices[i].tangent);
                }
                if (glm::length(vertices[i].tangent) < 0.99) {
                    if (printErrorMessage) {
                        utils::ConsoleWarning("assimpLoadMesh(): Input tangent for mesh " + meshName + " is not normalized");
                        printErrorMessage = false;
                    }
                    vertices[i].tangent = glm::normalize(vertices[i].tangent);
                }
                if (glm::length(vertices[i].bitangent) < 0.99) {
                    if (printErrorMessage) {
                        utils::ConsoleWarning("assimpLoadMesh(): Input bitangent for mesh " + meshName + " is not normalized");
                        printErrorMessage = false;
                    }
                    vertices[i].bitangent = glm::normalize(vertices[i].bitangent);
                }
            #endif
        }
    }

    /* Iterate over faces */
    for (size_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    Mesh temp(vertices, indices, hasNormals, hasUVs);
    temp.m_name = meshName;
    
    if (!hasUVs)
    {
        utils::ConsoleWarning("Mesh: [" + std::string(scene->mRootNode->mName.C_Str()) + " : " + std::string(temp.m_name) + "] doesn't have UV coordinates");
    }

    return temp;
}
