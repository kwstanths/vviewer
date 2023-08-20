#ifndef __Node_hpp__
#define __Node_hpp__

#include <vector>
#include <memory>

#include <math/Transform.hpp>

namespace vengine {

template<typename T>
class SceneNode {
public:
	SceneNode(Transform transform) : m_localTransform(transform) {};

	Transform m_localTransform;
	glm::mat4 m_modelMatrix = glm::mat4(1.0f);
	glm::vec3 getWorldPosition() const;

	SceneNode<T> * m_parent = nullptr;
	std::vector<std::shared_ptr<T>> m_children;

	std::shared_ptr<T> addChild(std::shared_ptr<T> node);

	void update();

	/* Get all nodes in a flat array */
	std::vector<std::shared_ptr<T>> getSceneObjectsArray();
	std::vector<std::shared_ptr<T>> getSceneObjectsArray(std::vector<glm::mat4>& modelMatrices);

	virtual void setModelMatrix(const glm::mat4& modelMatrix) = 0;

private:

};

}

#endif