#include "SceneGraph.hpp"

std::shared_ptr<SceneNode> SceneNode::addChild(std::shared_ptr<SceneObject> so, Transform transform)
{
	m_children.push_back(std::make_shared<SceneNode>(so, transform));
	m_children.back()->m_parent = this;
	return m_children.back();
}

void SceneNode::update()
{
	if (m_parent) {
		m_modelMatrix = m_parent->m_modelMatrix * m_localTransform.getModelMatrix();
	}
	else {
		m_modelMatrix = m_localTransform.getModelMatrix();
	}

	m_so->setModelMatrix(m_modelMatrix);

	for (auto&& child : m_children)
	{
		child->update();
	}
}

std::vector<std::shared_ptr<SceneObject>> SceneNode::getSceneObjects()
{
	std::vector<std::shared_ptr<SceneObject>> temp;
	temp.push_back(m_so);

	for (auto&& child : m_children)
	{
		auto childrenObjects = child->getSceneObjects();
		temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
	}

	return temp;
}

std::vector<std::shared_ptr<SceneObject>> SceneNode::getSceneObjects(std::vector<glm::mat4>& modelMatrices)
{
	std::vector<std::shared_ptr<SceneObject>> temp;
	temp.push_back(m_so);
	modelMatrices.push_back(m_modelMatrix);

	for (auto&& child : m_children)
	{
		std::vector<glm::mat4> childrenMatrices;
		auto childrenObjects = child->getSceneObjects(childrenMatrices);

		temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
		modelMatrices.insert(modelMatrices.end(), childrenMatrices.begin(), childrenMatrices.end());
	}

	return temp;
}
