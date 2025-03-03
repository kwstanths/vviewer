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

#include "dds.hpp"

#include "debug_tools/Console.hpp"

#include "core/io/FileTypes.hpp"
#include "core/StringUtils.hpp"
#include "core/ImageUtils.hpp"
#include "math/Transform.hpp"
#include "math/MathUtils.hpp"

#define SANITIZATION_CHECK
// #define DEBUG_MATERIAL_WRITE_EMBEDDED_IMAGES
 //#define DEBUG_MATERIAL_DUMP_ALL_PROPERTIES

namespace vengine
{

bool checkValid(glm::vec3 in)
{
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y) && !std::isnan(in.z) && !std::isinf(in.z);
}

bool checkValid(glm::vec2 in)
{
    return !std::isnan(in.x) && !std::isinf(in.x) && !std::isnan(in.y) && !std::isinf(in.y);
}

Mesh assimpLoadMesh(aiMesh *mesh, const aiScene *scene, const AssetInfo &info)
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
        if (!checkValid(vertices[i].position) && printErrorMessage) {
            debug_tools::ConsoleCritical("assimpLoadMesh(): Input vertex for mesh [" + meshName + "] is corrupted");
            printErrorMessage = false;
        }
#endif

        if (hasNormals) {
            vertices[i].normal = glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z));

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].normal) && printErrorMessage) {
                debug_tools::ConsoleCritical("assimpLoadMesh(): Input normal for mesh [" + meshName + "] is corrupted");
                printErrorMessage = false;
            }
#endif
        }

        if (mesh->mTextureCoords[0]) {
            /* Flip v coordinate vertically because of vulkan */
            vertices[i].uv = {mesh->mTextureCoords[0][i].x, 1.f - mesh->mTextureCoords[0][i].y};

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].uv) && printErrorMessage) {
                debug_tools::ConsoleCritical("assimpLoadMesh(): Input uv for mesh [" + meshName + "] is corrupted");
                printErrorMessage = false;
            }
#endif

        } else {
            vertices[i].uv = {0, 0};
        }

        vertices[i].color = {1, 1, 1};

        if (hasTangents) {
            vertices[i].tangent = glm::normalize(glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
            vertices[i].bitangent = glm::normalize(glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));

#ifdef SANITIZATION_CHECK
            if (!checkValid(vertices[i].tangent) || !checkValid(vertices[i].bitangent)) {
                if (printErrorMessage) {
                    debug_tools::ConsoleWarning("assimpLoadMesh(): Input tangent or bitangent for mesh [" + meshName +
                                                "] is corrupted. Attempting reconstruction from normal...");
                    printErrorMessage = false;
                }

                auto t1 = glm::cross(vertices[i].normal, Transform::WORLD_Z);
                auto t2 = glm::cross(vertices[i].normal, Transform::WORLD_X);
                if (glm::length(t1) > glm::length(t2)) {
                    vertices[i].tangent = glm::normalize(t1);
                } else {
                    vertices[i].tangent = glm::normalize(t2);
                }

                vertices[i].bitangent = glm::normalize(glm::cross(vertices[i].normal, vertices[i].tangent));
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

    Mesh temp(AssetInfo(meshName, info.filepath, info.source, AssetLocation::DISK_EMBEDDED), vertices, indices, hasNormals, hasUVs);

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

void assimpLoadNode(aiNode *node, const aiScene *scene, const AssetInfo &info, Tree<ImportedModelNode> &root)
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
        root.data().meshes.push_back(assimpLoadMesh(mesh, scene, info));
        root.data().materialIndices.push_back(mesh->mMaterialIndex);
    }

    /* Loop through all nodes attached to this node, and append their meshes */
    for (size_t i = 0; i < node->mNumChildren; i++) {
        assimpLoadNode(node->mChildren[i], scene, info, root.add());
    }
}

void assimpPrintAllMaterialProperties(const aiScene *scene, const aiMaterial* mat)
{
    debug_tools::ConsoleInfo("assimp material: " + std::string(mat->GetName().C_Str()) + " properties");
    for (unsigned int i = 0; i < mat->mNumProperties; ++i) {
        aiMaterialProperty *prop = mat->mProperties[i];

        if (prop->mType == aiPropertyTypeInfo::aiPTI_String) {
            aiString temp;
            aiReturn ret = mat->Get(prop->mKey.data, UINT_MAX, prop->mIndex, temp);
            std::cout << prop->mKey.C_Str() << " [" << temp.C_Str() << "]" << std::endl;
        }
        else if (prop->mType == aiPropertyTypeInfo::aiPTI_Double)
        {
            double temp;
            aiReturn ret = mat->Get(prop->mKey.data, UINT_MAX, prop->mIndex, temp);
            std::cout << prop->mKey.C_Str() << " [" << std::to_string(temp) << "]" << std::endl;
        } 
    }
}

Image *assimpLoadTextureData(const aiScene *scene,
                             const std::string &folder,
                             aiString texturePath,
                             const AssetInfo &info,
                             ColorSpace colorSpace)
{
    if (texturePath.length != 0) {
        const aiTexture *texture = scene->GetEmbeddedTexture(texturePath.data);
        if (texture != nullptr) {
            /* Embedded texture */
            if (texture->mHeight == 0) {
                /* Encoded */
                unsigned char *encodedData = reinterpret_cast<unsigned char *>(texture->pcData);

                int width, height, channels;
                unsigned char *textureData =
                    stbi_load_from_memory(encodedData, texture->mWidth, &width, &height, &channels, STBI_rgb_alpha);
                if (textureData == nullptr) {
                    debug_tools::ConsoleWarning("assimpLoadTexture(): Failed to load texture: " + std::string(texturePath.C_Str()) +
                                                " from memory");
                    return nullptr;
                }
#ifdef DEBUG_MATERIAL_WRITE_EMBEDDED_IMAGES
                stbi_write_png(texturePath.C_Str(), width, height, 4U, textureData, width * 4 * sizeof(unsigned char));
#endif
                Format format;
                if (colorSpace == ColorSpace::sRGB)
                    format = Format::R8G8B8A8_SRGB;
                else 
                    format = Format::R8G8B8A8_UNORM;
                bool hasAlpha = (channels < 4) ? false : true;
                return new Image(info, textureData, width, height, format, colorSpace, hasAlpha, true);
            } else {
                /* Not encoded */
                debug_tools::ConsoleWarning("assimpLoadTexture(): Not implemented");
                return nullptr;
            }
        } 
        else 
        {
            /* Parse from disk */
            std::string fullPath = folder + std::string(texturePath.C_Str());
#ifdef __linux__
            replaceAll(fullPath, "\\", "/");
#endif
            try {
                return new Image(AssetInfo(fullPath), colorSpace);
            } catch (std::exception &e) {
                debug_tools::ConsoleWarning("assimpLoadTexture(): Failed to load texture: " + fullPath + " from memory: " + e.what());
                return nullptr;
            }
        }
    }
    return nullptr;
}

std::vector<ImportedMaterial> assimpLoadMaterialsOBJ(const aiScene *scene,
                                                     const AssetInfo &info,
                                                     std::string folderPath,
                                                     std::string materialNamePrefix = "")
{
    std::vector<ImportedMaterial> materials(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *mat = scene->mMaterials[i];

#ifdef DEBUG_MATERIAL_DUMP_ALL_PROPERTIES
        assimpPrintAllMaterialProperties(scene, mat);
#endif

        ImportedMaterial &importedMaterial = materials[i];

        {
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);

            importedMaterial.info =
                AssetInfo(materialNamePrefix + ":" + std::string(name.C_Str()), info.filepath, info.source, AssetLocation::DISK_EMBEDDED);
        }

        importedMaterial.type = ImportedMaterialType::PBR_STANDARD;

        {
            aiColor4D diffuseColor;
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
            importedMaterial.albedo = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

            aiString diffuseColorTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), diffuseColorTexture);

            Image *image = assimpLoadTextureData(
                scene,
                folderPath,
                diffuseColorTexture,
                AssetInfo(importedMaterial.info.name + ":albedo", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                ColorSpace::sRGB);
            if (image != nullptr) 
            {
                importedMaterial.albedoTexture = ImportedTexture(image);
                if (isBlack(glm::vec3(importedMaterial.albedo))) 
                {
                    importedMaterial.albedo = glm::vec4(1, 1, 1, importedMaterial.albedo.a);
                }
                if (image->hasAlpha()) 
                {
                    importedMaterial.transparent = true;
                    if (importedMaterial.albedo.a == 0) {
                        importedMaterial.albedo.a = 1;
                    }
                }
            } 
        }

        /* Try to simulate a PBR material from the .mtl shading model illum 4 */
        aiColor4D Ks;
        mat->Get(AI_MATKEY_COLOR_SPECULAR, Ks);

        aiColor4D Ns;
        mat->Get(AI_MATKEY_SHININESS, Ns);

        float diffuseAmount = std::max(std::max(importedMaterial.albedo.r, importedMaterial.albedo.g), importedMaterial.albedo.b);
        float specularAmount = std::max(std::max(Ks.r, Ks.g), Ks.b);

        importedMaterial.metallic = 1 - diffuseAmount / (diffuseAmount + specularAmount);
        importedMaterial.roughness = 1 - Ns.r / 100;

        if (isBlack(glm::vec3(importedMaterial.albedo)) && specularAmount != 0) {
            importedMaterial.albedo = glm::vec4(Ks.r, Ks.g, Ks.b, importedMaterial.albedo.a);
        }

        {
            // Parses map_Bump
            aiString normalTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_HEIGHT, 0), normalTexture);

            Image *image = assimpLoadTextureData(
                scene,
                folderPath,
                normalTexture,
                AssetInfo(importedMaterial.info.name + ":normal", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                ColorSpace::LINEAR);
            if (image != nullptr) {
                importedMaterial.normalTexture = ImportedTexture(image);
            }
        }
    }
    return materials;
}

std::vector<ImportedMaterial> assimpLoadMaterialsGLTF(const aiScene *scene,
                                                      const AssetInfo &info,
                                                      std::string folderPath,
                                                      std::string materialNamePrefix = "")
{
    std::vector<ImportedMaterial> materials(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *mat = scene->mMaterials[i];

#ifdef DEBUG_MATERIAL_DUMP_ALL_PROPERTIES
        assimpPrintAllMaterialProperties(scene, mat);
#endif

        ImportedMaterial &importedMaterial = materials[i];

        {
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);

            importedMaterial.info =
                AssetInfo(materialNamePrefix + ":" + std::string(name.C_Str()), info.filepath, info.source, AssetLocation::DISK_EMBEDDED);
        }

        importedMaterial.type = ImportedMaterialType::PBR_STANDARD;

        {
            aiColor4D baseColor;
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, baseColor);
            importedMaterial.albedo = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);

            aiString baseColorTexture;
            mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &baseColorTexture);

            Image *image = assimpLoadTextureData(
                scene,
                folderPath,
                baseColorTexture,
                AssetInfo(importedMaterial.info.name + ":albedo", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                ColorSpace::sRGB);
            if (image != nullptr) {
                importedMaterial.albedoTexture = ImportedTexture(image);
                if (image->hasAlpha())
                    importedMaterial.transparent = true;
            }

            aiString alphaMode;
            mat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode);
            if (strcmp(alphaMode.C_Str(), "OPAQUE") != 0) {
                importedMaterial.transparent = true;
            }
        }

        {
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, importedMaterial.roughness);
            mat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, importedMaterial.metallic);

            aiString roughnessTexture;
            mat->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &roughnessTexture);
            Image *image =
                assimpLoadTextureData(scene,
                                      folderPath,
                                      roughnessTexture,
                                      AssetInfo(importedMaterial.info.name, info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                                      ColorSpace::LINEAR);
            if (image != nullptr) {
                Image* roughnessImage = extractChannel(
                    *image,
                    AssetInfo(importedMaterial.info.name + ":roughness", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                    1);
                if (roughnessImage != nullptr)
                    importedMaterial.roughnessTexture = ImportedTexture(roughnessImage);
                
                Image* metallicImage = extractChannel(
                    *image,
                    AssetInfo(importedMaterial.info.name + ":metallic", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                    2);
                if (metallicImage != nullptr)
                    importedMaterial.metallicTexture = ImportedTexture(metallicImage);
                
                delete image;
            }
        }

        {
            aiString occlusionTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_LIGHTMAP, 0), occlusionTexture);
            Image *image =
                assimpLoadTextureData(scene,
                                      folderPath,
                                      occlusionTexture,
                                      AssetInfo(importedMaterial.info.name, info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                                      ColorSpace::LINEAR);
            if (image != nullptr) {
                Image *aoImage = extractChannel(
                    *image,
                    AssetInfo(importedMaterial.info.name + ":ao", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                    1);
                if (aoImage != nullptr)
                    importedMaterial.aoTexture = ImportedTexture(aoImage);
                
                delete image;
            }
        }

        {
            aiColor3D emissiveColor;
            mat->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
            importedMaterial.emissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);

            mat->Get(AI_MATKEY_EMISSIVE_INTENSITY, importedMaterial.emissiveStrength);

            aiString emissiveTexture;
            mat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), emissiveTexture);
            Image *image = assimpLoadTextureData(
                scene,
                folderPath,
                emissiveTexture,
                AssetInfo(importedMaterial.info.name + ":emissive", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                ColorSpace::sRGB);
            if (image != nullptr) {
                importedMaterial.emissiveTexture = ImportedTexture(image);

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
            Image *image = assimpLoadTextureData(
                scene,
                folderPath,
                normalTexture,
                AssetInfo(importedMaterial.info.name + ":normal", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                ColorSpace::LINEAR);
            if (image != nullptr) {
                importedMaterial.normalTexture = ImportedTexture(image);
            }
        }
    }

    return materials;
}

std::vector<ImportedMaterial> assimpLoadMaterialsFBX(const aiScene *scene,
                                                      const AssetInfo &info,
                                                      std::string folderPath,
                                                      std::string materialNamePrefix = "")
{
    std::vector<ImportedMaterial> materials(scene->mNumMaterials);

    for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial *mat = scene->mMaterials[i];

#ifdef DEBUG_MATERIAL_DUMP_ALL_PROPERTIES
        assimpPrintAllMaterialProperties(scene, mat);
#endif

        ImportedMaterial &importedMaterial = materials[i];
        
        aiReturn ret;
        {
            aiString name;
            ret = mat->Get(AI_MATKEY_NAME, name);
            if (ret == aiReturn_SUCCESS)
            {
                importedMaterial.info = AssetInfo(
                    materialNamePrefix + ":" + std::string(name.C_Str()), info.filepath, info.source, AssetLocation::DISK_EMBEDDED);
            }
        }

        importedMaterial.type = ImportedMaterialType::PBR_STANDARD;

        {
            aiColor4D baseColor;
            ret = mat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);

            if (ret == aiReturn_SUCCESS)
                importedMaterial.albedo = glm::vec4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);

            aiString baseColorTexture;
            ret = mat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), baseColorTexture);
            if (ret == aiReturn_SUCCESS)
            {
                Image *image = assimpLoadTextureData(
                    scene,
                    folderPath,
                    baseColorTexture,
                    AssetInfo(importedMaterial.info.name + ":albedo", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                    ColorSpace::sRGB);
                if (image != nullptr) 
                {
                    importedMaterial.albedoTexture = {image};
                    if (image->hasAlpha()) 
                        importedMaterial.transparent = true;
                }
            }
        }

        {
            aiString normalTexture;
            ret = mat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), normalTexture);
            if (ret == aiReturn_SUCCESS)
            {
                Image *image = assimpLoadTextureData(
                    scene,
                    folderPath,
                    normalTexture,
                    AssetInfo(importedMaterial.info.name + ":normal", info.filepath, info.source, AssetLocation::DISK_EMBEDDED),
                    ColorSpace::LINEAR);
                if (image != nullptr) 
                {
                    importedMaterial.normalTexture = {image};
                }
            }
        }
    }

    return materials;
}


Tree<ImportedModelNode> assimpLoadModel(const AssetInfo &info)
{
    assert(aiGetVersionMajor() >= 5);
    assert(aiGetVersionMinor() >= 2);
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(info.filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene) {
        importer.FreeScene();
        throw std::runtime_error("Failed to load model: " + info.filepath);
    }

    Tree<ImportedModelNode> importedModel;
    assimpLoadNode(scene->mRootNode, scene, info, importedModel);

    importer.FreeScene();

    return importedModel;
}

Tree<ImportedModelNode> assimpLoadModel(const AssetInfo &info, std::vector<ImportedMaterial> &materials)
{
    assert(aiGetVersionMajor() >= 5);
    assert(aiGetVersionMinor() >= 2);
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(info.filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if (!scene) {
        importer.FreeScene();
        throw std::runtime_error("Failed to load model: " + info.filepath);
    }

    Tree<ImportedModelNode> importedModel;
    assimpLoadNode(scene->mRootNode, scene, info, importedModel);

    FileType fileType;
    std::string folderPath;
    std::string filenameWithoutExtension;
    {
        auto extensionPointPosition = info.filepath.find_last_of(".");
        auto lastDashPosition = info.filepath.find_last_of("/");

        /*
            e.g. filename:              /whatever/models/test.obj
            folderPath is:              /whatever/models/
            filenameWithoutExtension:   test
        */

        fileType = findFileType(info.filepath);
        folderPath = info.filepath.substr(0, lastDashPosition) + "/";
        filenameWithoutExtension = info.filepath.substr(lastDashPosition + 1, extensionPointPosition - lastDashPosition - 1);
    }

    switch (fileType) {
        case FileType::GLTF: {
            materials = assimpLoadMaterialsGLTF(scene, info, folderPath, filenameWithoutExtension);
            break;
        }
        case FileType::OBJ: {
            materials = assimpLoadMaterialsOBJ(scene, info, folderPath, filenameWithoutExtension);
            break;
        }
        case FileType::FBX: {
            materials = assimpLoadMaterialsFBX(scene, info, folderPath, filenameWithoutExtension);
            break;
        }
        default: {
            debug_tools::ConsoleWarning("AssimpLoadMaterials(): Unknown model type");
            break;
        }
    };  // namespace vengine

    importer.FreeScene();

    return importedModel;
}

}  // namespace vengine