#ifndef __Import_hpp__
#define __Import_hpp__

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include <rapidjson/document.h>

#include <glm/glm.hpp>

#include <math/Transform.hpp>

enum class ImportedSceneMaterialType {
	DIFFUSE = 0,
	DISNEY = 1,
	STACK = 2,
};
static std::unordered_map<std::string, ImportedSceneMaterialType> importedMaterialTypeNames = {
	{"DIFFUSE" , ImportedSceneMaterialType::DIFFUSE },
	{"DISNEY12", ImportedSceneMaterialType::DISNEY },
	{"DISNEY15", ImportedSceneMaterialType::DISNEY },
	{"STACK", ImportedSceneMaterialType::STACK },
	{"STACK12", ImportedSceneMaterialType::STACK },
	{"STACK15", ImportedSceneMaterialType::STACK }
};
enum class ImportedSceneLightType {
	POINT = 0,
	DISTANT = 1,
	QUAD = 2,
	SPHERE = 3,
	MESH = 4,
};
static std::unordered_map<std::string, ImportedSceneLightType> importedLightMaterialTypeNames = {
	{"POINT", ImportedSceneLightType::POINT },
	{"DISTANT", ImportedSceneLightType::DISTANT },
	{"QUAD", ImportedSceneLightType::QUAD },
	{"SPHERE", ImportedSceneLightType::SPHERE },
	{"MESH", ImportedSceneLightType::MESH },
};

struct ImportedSceneCamera {
	glm::vec3 position = {0, 0, 0};
	glm::vec3 target = {0, 0, -1};
	glm::vec3 up {0, 1, 0};
	float fov;
};

struct ImportedSceneMaterial {
	std::string name;
	ImportedSceneMaterialType type;

	std::string albedoTexture = "", roughnessTexture = "", metallicTexture = "", normalTexture = "";
	glm::vec3 albedo = {1, 1, 1};
	float roughness = 1.F, metallic = 0.0F;
	glm::vec2 scale = {1, 1};

	/* If it's a stack material, store here its directory, or if it's a zip, store the filename */
	std::string stackFile = "", stackDir = "";
};

struct ImportedSceneLightMaterial {
	std::string name;
	glm::vec3 color = {1, 1, 1};
	float intensity = 1.F;
};

struct ImportedSceneEnvironment {
	std::string path = "";
	glm::vec3 rotation = {0, 0, 0};
	int intensity = 1.F;
};

/* Scene node components */
struct ImportedSceneObjectMesh {
	std::string path = "";
	std::string material = "";
	int submesh = -1;
};
struct ImportedSceneObjectQuad {
	std::string material;
	float width = 1.F, height = 1.F;
};
struct ImportedSceneObjectSphere {
	std::string material;
	float radius = 1.F;
};
struct ImportedSceneObjectLight {
	ImportedSceneLightType type;
	std::string lightMaterial;
	float radius = 1.F, width = 1.F, height = 1.F;
	int submesh = -1;
	std::string path = "";
};

struct ImportedSceneObject {
	std::string name;
	Transform transform = Transform({0, 0, 0}, {1, 1, 1}, {0, 0, 0});
	std::optional<ImportedSceneObjectMesh> mesh = std::nullopt;
	std::optional<ImportedSceneObjectQuad> quad = std::nullopt;
	std::optional<ImportedSceneObjectSphere> sphere = std::nullopt;
	std::optional<ImportedSceneObjectLight> light = std::nullopt;
	std::vector<ImportedSceneObject> children;
};

std::string importScene(std::string filename, 
	ImportedSceneCamera& c, 
	std::unordered_map<std::string, ImportedSceneMaterial>& materials, 
	std::unordered_map<std::string, ImportedSceneLightMaterial>& lights,
	ImportedSceneEnvironment& e,
	std::vector<ImportedSceneObject>& sceneObjects
);

ImportedSceneCamera parseCamera(const rapidjson::Value& o);
ImportedSceneObject parseSceneObject(const rapidjson::Value& o);
std::optional<ImportedSceneObjectMesh> parseMesh(const rapidjson::Value& o);
std::optional<ImportedSceneObjectQuad> parseQuad(const rapidjson::Value& o);
std::optional<ImportedSceneObjectSphere> parseSphere(const rapidjson::Value& o);
std::optional<ImportedSceneObjectLight> parseLight(const rapidjson::Value& o);
ImportedSceneLightMaterial parseLightMaterial(const rapidjson::Value& o);
ImportedSceneMaterial parseMaterial(const rapidjson::Value& o);
ImportedSceneEnvironment parseEnvironment(const rapidjson::Value& o);

glm::vec2 parseVec2(const rapidjson::Value& o, std::string name, glm::vec2 defaultValue);
glm::vec3 parseVec3(const rapidjson::Value& o, std::string name, glm::vec3 defaultValue);
glm::vec4 parseVec4(const rapidjson::Value& o, std::string name, glm::vec4 defaultValue);
glm::quat parseRotation(const rapidjson::Value& o, std::string name, glm::quat defaultValue);

#endif