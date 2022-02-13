#ifndef __SceneGraph_hpp__
#define __SceneGraph_hpp__

#include <vector>
#include <memory>

#include <math/Transform.hpp>
#include "SceneObject.hpp"

class Node {
public:
	Node(std::shared_ptr<SceneObject> so, Transform transform) : m_so(so), m_localTransform(transform) {};

	Transform m_localTransform;
	glm::mat4 m_modelMatrix;

	std::vector<std::shared_ptr<Node>> m_children;

	Node* m_parent = nullptr;

	std::shared_ptr<SceneObject> m_so;

	std::shared_ptr<Node> addChild(std::shared_ptr<SceneObject> so, Transform transform);

	void update();

	std::vector<std::shared_ptr<SceneObject>> getSceneObjects();

private:
};

#endif