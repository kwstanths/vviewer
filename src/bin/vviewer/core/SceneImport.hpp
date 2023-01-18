#ifndef __SceneImport_hpp__
#define __SceneImport_hpp__

#include <string>
#include <vector>
#include <unordered_map>

#include <rapidjson/document.h>

#include <glm/glm.hpp>

struct ImportedSceneCamera {
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 up;
	float fov;
};

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
struct ImportedSceneMaterial {
	std::string name;
	ImportedSceneMaterialType type;

	/* If it's a disney or lambert material, store here its properties*/
	std::string albedoTexture = "";
	glm::vec3 albedoValue = {1, 1, 1};

	std::string roughnessTexture;
	float roughnessValue = 1;

	std::string metallicTexture;
	float metallicValue = 1;

	std::string normalTexture;

	float emissiveValue = 0;

	/* If it's a stack material, store here its directory, or if it's a zip, store the filename */
	std::string stackFile = "";
	std::string stackDir = "";
};

struct ImportedSceneEnvironment {
	std::string path = "";
};

struct ImportedSceneObjectMesh {
	std::string path = "";
	int submesh = -1;;
};

enum class ImportedScenePointLightType {
	POINT = 0,
	DISTANT = 1,
};
struct ImportedSceneObjectLight {
	ImportedScenePointLightType type;
	glm::vec3 color = {1, 1, 1};
	float intensity = 1.F;
};

struct ImportedSceneObject {
	std::string name;
	ImportedSceneObjectMesh * mesh = nullptr;
	std::string material = "";
	ImportedSceneObjectLight * light = nullptr;
	glm::vec3 position = {0, 0, 0};
	glm::vec3 scale = { 1, 1, 1 };
	glm::vec3 rotation = { 0, 0, 0 };

	std::vector<ImportedSceneObject> children;

	void destroy() {
		if (mesh != nullptr) delete mesh;
	}
};

std::string importScene(std::string filename, 
	ImportedSceneCamera& c, 
	std::vector<ImportedSceneMaterial>& materials, 
	ImportedSceneEnvironment& e,
	std::vector<ImportedSceneObject>& sceneObjects
);

ImportedSceneObject parseSceneObject(const rapidjson::Value& o);
ImportedSceneObjectMesh * parseMesh(const rapidjson::Value& o);
ImportedSceneObjectLight * parseLight(const rapidjson::Value& o);
void parseAlbedo(ImportedSceneMaterial& m, const rapidjson::Value& o);

#endif