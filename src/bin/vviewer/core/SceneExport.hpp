#ifndef __SceneExport_hpp__
#define __SceneExport_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include "Camera.hpp"
#include "SceneObject.hpp"
#include "Materials.hpp"
#include "Lights.hpp"

void exportJson(std::string name, 
	std::shared_ptr<Camera> sceneCamera, 
	std::shared_ptr<DirectionalLight> sceneLight,
	const std::vector<std::shared_ptr<SceneObject>>& sceneObjects, 
	EnvironmentMap * envMap,
	uint32_t width, uint32_t height, uint32_t samples);

void parseSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::unordered_set<Material*>& materials, std::string meshDirectory);

void addSceneObjectMesh(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::string meshDirectory);

void addSceneObjectTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t);

void addSceneObjectLight(rapidjson::Document& d, rapidjson::Value& v, Light * light, std::string type);

void addMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material, std::string materialsDirectory);

void addColor(rapidjson::Document& d, rapidjson::Value& v, glm::vec3 color);

#endif