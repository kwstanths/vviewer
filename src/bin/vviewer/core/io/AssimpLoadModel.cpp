#include "AssimpLoadModel.hpp"

#include <stdexcept>
#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/version.h>
#include <assimp/pbrmaterial.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <stb_image.h>
#include <stb_image_write.h>

#include "debug_tools/Console.hpp"

#include "math/Transform.hpp"
#include "math/MathUtils.hpp"

#define SANITIZATION_CHECK
// #define DEBUG_MATERIAL_WRITE_EMBEDDED_IMAGES
// #define DEBUG_MATERIAL_PRINT_IMPORTED_IMAGES

namespace vengine
{

enum class FileType {
    UNKNOWN = -1,
    OBJ = 0,
    GLTF = 1,
    FBX = 2,
};
std::unordered_map<std::string, FileType> fileTypes = {
    {"obj", FileType::OBJ},
    {"OBJ", FileType::OBJ},
    {"gltf", FileType::GLTF},
    {"gltf2", FileType::GLTF},
    {"GLTF", FileType::GLTF},
    {"glb", FileType::GLTF},
    {"GLB", FileType::GLTF},
    {"fbx", FileType::FBX},
    {"FBX", FileType::FBX},
};

FileType detectFileType(std::string extension)
{
    auto itr = fileTypes.find(extension);
    if (itr == fileTypes.end()) {
        return FileType::UNKNOWN;
    }
    return itr->second;
}

bool checkValid(glm::vec3 in)
{
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y) && !std::isnan(in.z) && !std::isinf(in.z);
}

bool checkValid(glm::vec2 in)
{
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y);
}

Mesh assimpLoadMesh(aiMesh *mesh, const aiScene *scene)
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
        vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

#ifdef SANITIZATION_CHECK
        if (!checkValid(vertices[i].position))
            debug_tools::ConsoleCritical("assimpLoadMesh(): Input vertex for mesh [" + meshName + "] is corrupted");
#endif

        if (hasNormals) {
            vertices[i].normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].normal))
                debug_tools::ConsoleCritical("assimpLoadMesh(): Input normal for mesh [" + meshName + "] is corrupted");
            if (glm::length(vertices[i].normal) < 0.99) {
                debug_tools::ConsoleCritical("assimpLoadMesh(): Input normal for mesh [" + meshName + "] is not normalized");
                vertices[i].normal = glm::normalize(vertices[i].normal);
            }
#endif
        }

        if (mesh->mTextureCoords[0]) {
            /* Flip v coordinate vertically because of vulkan */
            vertices[i].uv = {mesh->mTextureCoords[0][i].x, 1.f - mesh->mTextureCoords[0][i].y};

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].uv))
                debug_tools::ConsoleCritical("assimpLoadMesh(): Input uv for mesh [" + meshName + "] is corrupted");
#endif

        } else {
            vertices[i].uv = {0, 0};
        }

        vertices[i].color = {1, 1, 1};

        if (hasTangents) {
            vertices[i].tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertices[i].bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].tangent) || !checkValid(vertices[i].bitangent)) {
                if (printErrorMessage) {
                    debug_tools::ConsoleWarning("assimpLoadMesh(): Input tangent or bitangent for mesh [" + meshName +
                                                "] is corrupted. Attempting reconstruction from normal...");
                    printErrorMessage = false;
                }

                auto t1 = glm::cross(vertices[i].normal, Transform::Z);
                auto t2 = glm::cross(vertices[i].normal, Transform::X);
                if (glm::length(t1) > glm::length(t2)) {
                    vertices[i].tangent = t1;
                } else {
                    vertices[i].tangent = t2;
                }

                vertices[i].bitangent = glm::cross(vertices[i].normal, vertices[i].tangent);
            }
            if (glm::length(vertices[i].tangent) < 0.99) {
                if (printErrorMessage) {
                    debug_tools::ConsoleWarning("assimpLoadMesh(): Input tangent for mesh [" + meshName + "] is not normalized");
                    printErrorMessage = false;
                }
                vertices[i].tangent = glm::normalize(vertices[i].tangent);
            }
            if (glm::length(vertices[i].bitangent) < 0.99) {
                if (printErrorMessage) {
                    debug_tools::ConsoleWarning("assimpLoadMesh(): Input bitangent for mesh [" + meshName + "] is not normalized");
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

    Mesh temp(meshName, vertices, indices, hasNormals, hasUVs);

    if (!hasUVs) {
        debug_tools::ConsoleWarning("Mesh: [" + std::string(scene->mRootNode->mName.C_Str()) + " : " + std::string(temp.name()) +
                                    "] doesn't have UV coordinates");
    }

    return temp;
}

glm::mat4 getTransformation(aiNode *node)
{
    return glm::transpose(glm::make_mat4(&node->mTransformation.a1));
}

void assimpLoadNode(aiNode *node, const aiScene *scene, Tree<ImportedModelNode> &root)
{
    root.data().name = std::string(node->mName.C_Str());

    /* Decompose transformation matrix to separate components */
    glm::vec3 scale, translation, skew;
    glm::quat rotation;
    glm::vec4 perspective;
    glm::decompose(getTransformation(node), scale, rotation, translation, skew, perspective);
    root.data().transform = Transform(translation, scale, rotation);

    /* Loop through all meshes in this node */
    for (size_t i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        root.data().meshes.push_back(assimpLoadMesh(mesh, scene));
        root.data().materialIndices.push_back(mesh->mMaterialIndex);
    }

    /* Loop through all nodes attached to this node, and append their meshes */
    for (size_t i = 0; i < node->mNumChildren; i++) {
        assimpLoadNode(node->mChildren[i], scene, root.add());
    }
}

uint8_t *assimpLoadTextureData(const aiScene *scene,
                               std::string relativePath,
                               aiString texturePath,
                               int &width,
                               int &height,
                               int &channels)
{
    if (texturePath.length != 0) {
        const aiTexture *texture = scene->GetEmbeddedTexture(texturePath.data);
        if (texture != nullptr) {
            /* Embedded texture */
            if (texture->mHeight == 0) {
                /* Encoded */
                unsigned char *encodedData = reinterpret_cast<unsigned char *>(texture->pcData);

                unsigned char *textureData =
                    stbi_load_from_memory(encodedData, texture->mWidth, &width, &height, nullptr, STBI_rgb_alpha);
                channels = 4;
                if (textureData == nullptr) {
                    debug_tools::ConsoleWarning("assimpLoadTexture(): Failed to load texture: " + std::string(texturePath.C_Str()) +
                                                " from memory");
                }
#ifdef DEBUG_MATERIAL_WRITE_EMBEDDED_IMAGES
                stbi_write_png(texturePath.C_Str(), width, height, 4U, textureData, width * 4 * sizeof(unsigned char));
#endif
                return textureData;

            } else {
                /* Not encoded */
                /* TODO */
                assert(0 == 1);
            }
        } else {
            /* Parse from disk */
            std::string fullPath = relativePath + std::string(texturePath.C_Str());
            unsigned char *textureData = stbi_load(fullPath.c_str(), &width, &height, nullptr, STBI_rgb_alpha);
            channels = 4;
            if (textureData == nullptr) {
                debug_tools::ConsoleWarning("assimpLoadTexture(): Failed to load texture: " + fullPath + " from memory");
            }
            return textureData;
        }
    }
    return nullptr;
}

std::shared_ptr<Image<uint8_t>> assimpCreateImage(std::string name,
                                                  std::string filepath,
                                                  uint8_t *data,
                                                  int width,
                                                  int height,
                                                  int channels,
                                                  ColorSpace colorSpace,
                                                  int channel = -1)
{
    if (channel == -1) {
#ifdef DEBUG_MATERIAL_PRINT_IMPORTED_IMAGES
        debug_tools::ConsoleInfo("Loaded image: " + name);
#endif
        return std::make_shared<Image<uint8_t>>(name, filepath, data, width, height, channels, colorSpace, true);
    } else {
        uint8_t *channelData = new uint8_t[width * height];
        uint32_t index = 0;
        for (uint32_t i = 0; i < height; i++) {
            for (uint32_t j = 0; j < width; j++) {
                channelData[index++] = data[width * channels * i + j * channels + channel];
            }
        }
#ifdef DEBUG_MATERIAL_PRINT_IMPORTED_IMAGES
        debug_tools::ConsoleInfo("Loaded image: " + name);
#endif
        return std::make_shared<Image<uint8_t>>(name, filepath, channelData, width, height, 1, colorSpace, true);
    }
}

std::vector<ImportedMaterial> assimpLoadMaterialsGLTF(const aiScene *scene,
                                                      std::string filename,
                                                      std::string folderPath,
                                                      std::string textureNamePrefix = "")
{
    std::vector<ImportedMaterial> materials;

    for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *mat = scene->mMaterials[i];

        ImportedMaterial importedMaterial;

        {
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);
            importedMaterial.name = textureNamePrefix + ":" + std::string(name.C_Str());
            importedMaterial.filepath = filename;
        }

        importedMaterial.type = ImportedMaterialType::PBR_STANDARD;

        {
            aiColor3D baseColor;
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, baseColor);
            importedMaterial.albedo = glm::vec4(baseColor.r, baseColor.g, baseColor.b, 0);

            aiString baseColorTexture;
            mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &baseColorTexture);

            int width, height, channels;
            auto *texData = assimpLoadTextureData(scene, folderPath, baseColorTexture, width, height, channels);
            if (texData != nullptr) {
                ImportedTexture baseColorTex;
                baseColorTex.type = ImportedTextureType::EMBEDDED;
                baseColorTex.name = importedMaterial.name + ":albedo";
                baseColorTex.image =
                    assimpCreateImage(baseColorTex.name, filename, texData, width, height, channels, ColorSpace::sRGB);
                importedMaterial.albedoTexture = baseColorTex;
            }
        }

        {
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, importedMaterial.roughness);
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, importedMaterial.metallic);

            aiString roughnessTexture;
            mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &roughnessTexture);
            int width, height, channels;
            auto *texData = assimpLoadTextureData(scene, folderPath, roughnessTexture, width, height, channels);
            if (texData != nullptr) {
                ImportedTexture roughnessTex;
                roughnessTex.type = ImportedTextureType::EMBEDDED;
                roughnessTex.name = importedMaterial.name + ":roughness";
                roughnessTex.image =
                    assimpCreateImage(roughnessTex.name, filename, texData, width, height, channels, ColorSpace::LINEAR, 1);
                importedMaterial.roughnessTexture = roughnessTex;
            }
            if (texData != nullptr) {
                ImportedTexture metallicTex;
                metallicTex.type = ImportedTextureType::EMBEDDED;
                metallicTex.name = importedMaterial.name + ":metallic";
                metallicTex.image =
                    assimpCreateImage(metallicTex.name, filename, texData, width, height, channels, ColorSpace::LINEAR, 2);
                importedMaterial.metallicTexture = metallicTex;
            }
        }

        {
            aiString occlusionTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0), occlusionTexture);
            int width, height, channels;
            auto *texData = assimpLoadTextureData(scene, folderPath, occlusionTexture, width, height, channels);
            if (texData != nullptr) {
                ImportedTexture aoTex;
                aoTex.type = ImportedTextureType::EMBEDDED;
                aoTex.name = importedMaterial.name + ":ao";
                aoTex.image = assimpCreateImage(aoTex.name, filename, texData, width, height, channels, ColorSpace::LINEAR);
                importedMaterial.aoTexture = aoTex;
            }
        }

        {
            aiColor3D emissiveColor;
            mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
            importedMaterial.emissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);

            mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, importedMaterial.emissiveStrength);

            aiString emissiveTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), emissiveTexture);
            int width, height, channels;
            auto *texData = assimpLoadTextureData(scene, folderPath, emissiveTexture, width, height, channels);
            if (texData != nullptr) {
                ImportedTexture emissiveTex;
                emissiveTex.type = ImportedTextureType::EMBEDDED;
                emissiveTex.name = importedMaterial.name + ":emissive";
                emissiveTex.image = assimpCreateImage(emissiveTex.name, filename, texData, width, height, channels, ColorSpace::sRGB);
                importedMaterial.emissiveTexture = emissiveTex;

                /* If it has en emissive texture, then make sure that the color and strength are not zero */
                if (isBlack(importedMaterial.emissiveColor)) {
                    importedMaterial.emissiveColor = glm::vec3(1.0F);
                }
                if (importedMaterial.emissiveStrength == 0.0F) {
                    importedMaterial.emissiveStrength = 1.0F;
                }
            }
        }

        {
            aiString normalTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normalTexture);
            int width, height, channels;
            auto *texData = assimpLoadTextureData(scene, folderPath, normalTexture, width, height, channels);
            if (texData != nullptr) {
                ImportedTexture normalTex;
                normalTex.type = ImportedTextureType::EMBEDDED;
                normalTex.name = importedMaterial.name + ":normal";
                normalTex.image = assimpCreateImage(normalTex.name, filename, texData, width, height, channels, ColorSpace::LINEAR);
                importedMaterial.normalTexture = normalTex;
            }
        }

        materials.push_back(importedMaterial);
    }

    return materials;
}

Tree<ImportedModelNode> assimpLoadModel(std::string filename)
{
    assert(aiGetVersionMajor() >= 5);
    assert(aiGetVersionMinor() >= 2);
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene) {
        importer.FreeScene();
        throw std::runtime_error("Failed to load model: " + filename);
    }

    Tree<ImportedModelNode> importedModel;
    assimpLoadNode(scene->mRootNode, scene, importedModel);

    importer.FreeScene();

    return importedModel;
}

Tree<ImportedModelNode> assimpLoadModel(std::string filename, std::vector<ImportedMaterial> &materials)
{
    assert(aiGetVersionMajor() >= 5);
    assert(aiGetVersionMinor() >= 2);
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene) {
        importer.FreeScene();
        throw std::runtime_error("Failed to load model: " + filename);
    }

    Tree<ImportedModelNode> importedModel;
    assimpLoadNode(scene->mRootNode, scene, importedModel);

    FileType fileType;
    std::string folderPath;
    std::string filenamePure;
    {
        auto extensionPointPosition = filename.find_last_of(".");
        auto lastDashPosition = filename.find_last_of("/");

        fileType = detectFileType(filename.substr(extensionPointPosition + 1));
        folderPath = filename.substr(0, lastDashPosition) + "/";
        filenamePure = filename.substr(lastDashPosition + 1, extensionPointPosition - lastDashPosition - 1);
    }

    switch (fileType) {
        case FileType::GLTF: {
            materials = assimpLoadMaterialsGLTF(scene, filename, folderPath, filenamePure);
            break;
        }
        case FileType::OBJ: {
            debug_tools::ConsoleCritical("AssimpLoadMaterials(): .obj materials not implemented");
            break;
        }
        case FileType::FBX: {
            debug_tools::ConsoleCritical("AssimpLoadMaterials(): .fbx materials not implemented");
            break;
        }
        case FileType::UNKNOWN: {
            debug_tools::ConsoleWarning("AssimpLoadMaterials(): Unknown model type");
            break;
        }
    };  // namespace vengine

    importer.FreeScene();

    return importedModel;
}

}  // namespace vengine