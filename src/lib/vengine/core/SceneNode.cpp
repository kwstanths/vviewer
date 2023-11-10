#include "SceneNode.hpp"

#include <iostream>
#include <algorithm>

#include <math/MathUtils.hpp>

#include "SceneObject.hpp"

namespace vengine
{

template <>
glm::vec3 SceneNode<SceneObject>::worldPosition() const
{
    return getTranslation(m_modelMatrix);
}

template <>
SceneObject *SceneNode<SceneObject>::addChild(SceneObject *node)
{
    m_children.push_back(node);
    m_children.back()->parent() = this;
    return m_children.back();
}

template <>
void SceneNode<SceneObject>::removeChild(SceneObject *node)
{
    m_children.erase(std::remove(m_children.begin(), m_children.end(), node), m_children.end());
}

template <>
void SceneNode<SceneObject>::update()
{
    m_modelMatrixChanged = false;
    if (m_parent) {
        if (m_parent->modelMatrixChanged() || m_localTransformDirty) {
            m_modelMatrix = m_parent->m_modelMatrix * m_localTransform.getModelMatrix();
            m_modelMatrixChanged = true;
            m_localTransformDirty = false;
        }
    } else if (m_localTransformDirty) {
        m_modelMatrix = m_localTransform.getModelMatrix();
        m_modelMatrixChanged = true;
        m_localTransformDirty = false;
    }

    if (m_modelMatrixChanged) {
        setModelMatrix(m_modelMatrix);
    }

    for (auto &&child : m_children) {
        child->update();
    }
}

template <>
std::vector<SceneObject *> SceneNode<SceneObject>::getSceneObjectsArray()
{
    std::vector<SceneObject *> temp;

    for (auto &&child : m_children) {
        temp.push_back(child);
        auto childrenObjects = child->getSceneObjectsArray();

        temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
    }

    return temp;
}

template <>
std::vector<SceneObject *> SceneNode<SceneObject>::getSceneObjectsArray(std::vector<glm::mat4> &modelMatrices)
{
    std::vector<SceneObject *> temp;

    for (auto &&child : m_children) {
        temp.push_back(child);
        modelMatrices.push_back(child->m_modelMatrix);

        std::vector<glm::mat4> childrenMatrices;
        auto childrenObjects = child->getSceneObjectsArray(childrenMatrices);

        temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
        modelMatrices.insert(modelMatrices.end(), childrenMatrices.begin(), childrenMatrices.end());
    }

    return temp;
}

}  // namespace vengine
