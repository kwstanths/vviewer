#ifndef __SceneGraph_hpp__
#define __SceneGraph_hpp__

#include <vector>
#include <memory>

#include <math/Transform.hpp>
#include "SceneObject.hpp"

class SceneNode {
public:
	SceneNode(std::shared_ptr<SceneObject> so, Transform transform) : m_so(so), m_localTransform(transform) {};

	Transform m_localTransform;
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);
	glm::vec3 getWorldPosition() const;

	SceneNode* m_parent = nullptr;
	std::vector<std::shared_ptr<SceneNode>> m_children;

	std::shared_ptr<SceneObject> m_so;

	std::shared_ptr<SceneNode> addChild(std::shared_ptr<SceneObject> so, Transform transform);

	void update();

	std::vector<std::shared_ptr<SceneObject>> getSceneObjects();
	std::vector<std::shared_ptr<SceneObject>> getSceneObjects(std::vector<glm::mat4>& modelMatrices);

private:

};

#endif