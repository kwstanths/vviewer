#ifndef __Export_hpp__
#define __Export_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include "Camera.hpp"
#include "SceneObject.hpp"
#include "Materials.hpp"
#include "Lights.hpp"

struct ExportRenderParams {
	std::string name;
	std::vector<std::string> fileTypes;
	
	uint32_t width, height;
	uint32_t samples, depth, rdepth;

	bool hideBackground;
	glm::vec3 backgroundColor;
	float tessellation;
	bool parallax;

	float focalLength;
	uint32_t apertureSides;
	float aperture;
};

void exportJson(const ExportRenderParams& renderParams,
	std::shared_ptr<Camera> sceneCamera, 
	const std::vector<std::shared_ptr<SceneObject>>& sceneObjects, 
	std::shared_ptr<EnvironmentMap> envMap);

void parseSceneObject(rapidjson::Document& d, 
	rapidjson::Value& v, 
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::unordered_set<Material*>& materials, 
	std::unordered_set<LightMaterial*>& lightMaterials,
	std::string meshDirectory);

void addTransform(rapidjson::Document& d, 
	rapidjson::Value& v, 
	const Transform& t);

void addMesh(rapidjson::Document& d, 
	rapidjson::Value& v, 
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::string meshDirectory,
	std::string materialName);

void addMeshLight(rapidjson::Document& d, 
	rapidjson::Value& v,
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::string meshDirectory,
	std::string materialName);

void addMeshComponent(rapidjson::Document& d, 
	rapidjson::Value& v,
	const std::shared_ptr<SceneObject>& sceneObject, 
	std::string meshDirectory,
	std::string materialName);

void addAnalyticalLight(rapidjson::Document& d, rapidjson::Value& v, Light * light, std::string type);

void addMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material, std::string materialsDirectory);

void addLightMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material);

void addLightMaterial(rapidjson::Document& d, rapidjson::Value& v, const LightMaterial* material);

void addVec3(rapidjson::Document& d, rapidjson::Value& v, std::string name, glm::vec3 value);

bool isMaterialEmissive(const Material * material);

#endif