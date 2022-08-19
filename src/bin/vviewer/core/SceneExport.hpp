#ifndef __SceneExport_hpp__
#define __SceneEXport_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include "Camera.hpp"
#include "SceneObject.hpp"
#include "Materials.hpp"

void exportJson(std::string name, std::shared_ptr<Camera> sceneCamera, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects, EnvironmentMap * envMap);

void parseSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, std::unordered_set<Material*>& materials, std::string meshDirectory);

void addJsonSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, const Transform& t, std::string meshDirectory);

void addJsonSceneTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t);

void addJsonSceneMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material, std::string materialsDirectory);

void addJsonSceneColor(rapidjson::Document& d, rapidjson::Value& v, glm::vec4 color);

#endif