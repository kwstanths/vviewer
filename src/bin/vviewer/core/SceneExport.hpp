#ifndef __SceneExport_hpp__
#define __SceneEXport_hpp__

#include <iostream>
#include <vector>
#include <unordered_set>

#include <rapidjson/document.h>

#include "Camera.hpp"
#include "SceneGraph.hpp"
#include "Materials.hpp"

void exportJson(std::string name, std::shared_ptr<Camera> sceneCamera, const std::vector<std::shared_ptr<SceneNode>>& sceneObjects, EnvironmentMap * envMap);

void parseSceneNode(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneNode>& sceneNode, std::unordered_set<Material*>& materials);

void addJsonSceneObject(rapidjson::Document& d, rapidjson::Value& v, const std::shared_ptr<SceneObject>& sceneObject, const Transform& t);

void addJsonSceneTransform(rapidjson::Document& d, rapidjson::Value& v, const Transform& t);

void addJsonSceneMaterial(rapidjson::Document& d, rapidjson::Value& v, const Material* material);

void addJsonSceneColor(rapidjson::Document& d, rapidjson::Value& v, glm::vec4 color);

void addJsonSceneTexture(rapidjson::Document& d, rapidjson::Value& v, Texture* texture);

#endif