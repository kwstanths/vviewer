#include "SceneGraph.hpp"

std::shared_ptr<Node> Node::addChild(std::shared_ptr<SceneObject> so, Transform transform)
{
	m_children.push_back(std::make_shared<Node>(so, transform));
	m_children.back()->m_parent = this;
	return m_children.back();
}

void Node::update()
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

std::vector<std::shared_ptr<SceneObject>> Node::getSceneObjects()
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
