#include "AssimpLoadModel.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

std::vector<Mesh> assimpLoadModel(std::string filename)
{
    Assimp::Importer importer;

    const aiScene * scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
    if (!scene) {
        throw std::runtime_error("Failed to load model: " + filename);
    }

    return assimpLoadNode(scene->mRootNode, scene);
}

std::vector<Mesh> assimpLoadNode(aiNode * node, const aiScene * scene)
{
    std::vector<Mesh> meshList;

    /* Loop through all meshes in this node */
    for (size_t i = 0; i < node->mNumMeshes; i++) {
        meshList.push_back(assimpLoadMesh(scene->mMeshes[node->mMeshes[i]], scene));
    }

    /* Loop through all nodes attached to this node, and append their meshes */
    for (size_t i = 0; i < node->mNumChildren; i++) {
        std::vector<Mesh> childrenMeshes = assimpLoadNode(node->mChildren[i], scene);
        meshList.insert(meshList.end(), childrenMeshes.begin(), childrenMeshes.end());
    }

    return meshList;
}

Mesh assimpLoadMesh(aiMesh * mesh, const aiScene * scene)
{
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    bool hasNormals = mesh->HasNormals();
    bool hasUVs = mesh->HasTextureCoords(0);

    vertices.resize(mesh->mNumVertices);
    for (size_t i = 0; i < mesh->mNumVertices; i++) {
        vertices[i].position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

        if (hasNormals) {
            vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }

        if (mesh->mTextureCoords[0]) {
            vertices[i].uv = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        } else {
            vertices[i].uv = { 0, 0 };
        }

        vertices[i].color = { 1, 1, 1 };
    }

    /* Iterate over faces */
    for (size_t i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return Mesh(vertices, indices, hasNormals, hasUVs);
}
