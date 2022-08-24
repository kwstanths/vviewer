#ifndef __SceneImport_hpp__
#define __SceneImport_hpp__

#include <string>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

struct ImportedSceneCamera {
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 up;
	float fov;
};

typedef enum ImportedSceneMaterialType {
	DIFFUSE = 0,
	DISNEY = 1,
	STACK = 2,
};
static std::unordered_map<std::string, ImportedSceneMaterialType> importedMaterialTypeNames = {
	{"DIFFUSE" , DIFFUSE },
	{"DISNEY12", DISNEY },
	{"DISNEY15", DISNEY },
	{"STACK12", STACK },
	{"STACK15", STACK}
};
struct ImportedSceneMaterial {
	std::string name;
	ImportedSceneMaterialType type;
	std::string albedo;
	std::string roughness;
	std::string metallic;
	std::string normal;
	std::string ao;
	std::string emissive;
	std::string stackDir;
};

struct ImportedSceneEnvironment {
	std::string path;
};

struct ImportedSceneObject {
	std::string name;
	std::string path;
	std::string material;
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;
};

std::string importScene(std::string filename, 
	ImportedSceneCamera& c, 
	std::vector<ImportedSceneMaterial>& materials, 
	ImportedSceneEnvironment& e,
	std::vector<ImportedSceneObject>& sceneObjects
	);

#endif