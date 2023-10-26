#ifndef __ImportTypes_hpp__
#define __ImportTypes_hpp__

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include <vengine/core/Scene.hpp>
#include <vengine/core/Image.hpp>
#include <vengine/core/Mesh.hpp>
#include <vengine/core/Lights.hpp>
#include <vengine/math/Transform.hpp>
#include <vengine/utils/Tree.hpp>

namespace vengine
{

enum class ImportedMaterialType {
    LAMBERT = 0,
    PBR_STANDARD = 1,
    STACK = 2,
    EMBEDDED = 3,
};
enum class ImportedTextureType {
    DISK_FILE = 0,
    EMBEDDED = 1,
};

struct ImportedModelNode {
    std::string name;

    std::vector<Mesh> meshes;
    Transform transform;
    std::vector<uint32_t> materialIndices;
};

struct ImportedTexture {
    ImportedTextureType type;
    std::string name;
    std::shared_ptr<Image<uint8_t>> image = nullptr;
};

struct ImportedMaterial {
    std::string name;
    std::string filepath;
    ImportedMaterialType type;

    glm::vec4 albedo = glm::vec4(1, 1, 1, 1);
    std::optional<ImportedTexture> albedoTexture = std::nullopt;

    float roughness = 0.5F;
    std::optional<ImportedTexture> roughnessTexture = std::nullopt;

    float metallic = 0.5F;
    std::optional<ImportedTexture> metallicTexture = std::nullopt;

    float ao = 1.0F;
    std::optional<ImportedTexture> aoTexture = std::nullopt;

    glm::vec3 emissiveColor = {0, 0, 0};
    std::optional<ImportedTexture> emissiveTexture = std::nullopt;
    float emissiveStrength = 1.0F;

    std::optional<ImportedTexture> normalTexture = std::nullopt;

    glm::vec2 scale = {1, 1};
};

struct ImportedCamera {
    glm::vec3 position = {0, 0, 0};
    glm::vec3 target = {0, 0, -1};
    glm::vec3 up{0, 1, 0};
    float fov = 60.F;
};

struct ImportedLightMaterial {
    std::string name;
    glm::vec3 color = {1, 1, 1};
    float intensity = 1.F;
};

struct ImportedEnvironment {
    std::string path = "";
    glm::vec3 backgroundColor = {0, 0, 0};
    EnvironmentType environmentType = EnvironmentType::SOLID_COLOR;
};

/* Scene components */
struct ImportedSceneObjectMesh {
    std::string path = "";
    std::string submesh = "";
};

struct ImportedSceneObjectMaterial {
    std::string name = "";
};

struct ImportedSceneObjectLight {
    LightType type;
    std::string lightMaterial;
};

struct ImportedSceneObject {
    std::string name;
    Transform transform = Transform({0, 0, 0}, {1, 1, 1}, {0, 0, 0});
    std::optional<ImportedSceneObjectMesh> mesh = std::nullopt;
    std::optional<ImportedSceneObjectMaterial> material = std::nullopt;
    std::optional<ImportedSceneObjectLight> light = std::nullopt;
};

}  // namespace vengine

#endif