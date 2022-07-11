#include "SceneGraph.hpp"

#include <iostream>
#include "SceneObject.hpp"

glm::vec3 SceneNode<SceneObject>::getWorldPosition() const
{
	return glm::vec3(m_modelMatrix[3][0], m_modelMatrix[3][1], m_modelMatrix[3][2]);
}

std::shared_ptr<SceneObject> SceneNode<SceneObject>::addChild(std::shared_ptr<SceneObject> node)
{
	m_children.push_back(node);
	m_children.back()->m_parent = this;
	return m_children.back();
}

void SceneNode<SceneObject>::update()
{
	if (m_parent) {
		m_modelMatrix = m_parent->m_modelMatrix * m_localTransform.getModelMatrix();
	}
	else {
		m_modelMatrix = m_localTransform.getModelMatrix();
	}

	setModelMatrix(m_modelMatrix);

	for (auto&& child : m_children)
	{
		child->update();
	}
}

std::vector<std::shared_ptr<SceneObject>> SceneNode<SceneObject>::getSceneObjects()
{
	std::vector<std::shared_ptr<SceneObject>> temp;

	for (auto&& child : m_children)
	{
		temp.push_back(child);
		auto childrenObjects = child->getSceneObjects();

		temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
	}

	return temp;
}

std::vector<std::shared_ptr<SceneObject>> SceneNode<SceneObject>::getSceneObjects(std::vector<glm::mat4>& modelMatrices)
{
	std::vector<std::shared_ptr<SceneObject>> temp;

	for (auto&& child : m_children)
	{
		temp.push_back(child);
		modelMatrices.push_back(child->m_modelMatrix);

		std::vector<glm::mat4> childrenMatrices;
		auto childrenObjects = child->getSceneObjects(childrenMatrices);

		temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
		modelMatrices.insert(modelMatrices.end(), childrenMatrices.begin(), childrenMatrices.end());
	}

	return temp;
}
