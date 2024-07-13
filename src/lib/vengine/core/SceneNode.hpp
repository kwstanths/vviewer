#ifndef __Node_hpp__
#define __Node_hpp__

#include <vector>
#include <memory>
#include <algorithm>

#include <vengine/math/Transform.hpp>
#include <vengine/math/MathUtils.hpp>

namespace vengine
{

template <class T>
class SceneNode
{
public:
    SceneNode(Transform transform)
        : m_localTransform(transform){};

    /* Get set local transform */
    const Transform &localTransform() const { return m_localTransform; }
    void setLocalTransform(const Transform &transform)
    {
        m_localTransform = transform;
        m_localTransformDirty = true;
        transformChanged();
    }

    /* notify transform change */
    virtual void transformChanged(){};

    /* get set world space model matrix */
    const glm::mat4 &modelMatrix() const { return m_modelMatrix; }
    virtual void setModelMatrix(const glm::mat4 &modelMatrix) = 0;

    /* get node world position */
    glm::vec3 worldPosition() const { return getTranslation(m_modelMatrix); }

    /* get parent node */
    const SceneNode<T> *parent() const { return m_parent; }
    SceneNode<T> *&parent() { return m_parent; }

    /* get node children */
    const std::vector<T *> &children() const { return m_children; }

    /* add child */
    T *addChild(T *node)
    {
        m_children.push_back(node);
        m_children.back()->parent() = this;
        return m_children.back();
    }

    /* remove child */
    void removeChild(T *node) { m_children.erase(std::remove(m_children.begin(), m_children.end(), node), m_children.end()); }

    /* update node */
    void update()
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

    /* Get all nodes in a flat array */
    std::vector<T *> getSceneNodesFlat()
    {
        std::vector<T *> temp;

        for (auto &&child : m_children) {
            temp.push_back(child);
            auto childrenObjects = child->getSceneNodesFlat();

            temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
        }

        return temp;
    }

    /* get all nodes in a flat array, and their world space model matrices */
    std::vector<T *> getSceneNodesFlat(std::vector<glm::mat4> &modelMatrices)
    {
        std::vector<T *> temp;

        for (auto &&child : m_children) {
            temp.push_back(child);
            modelMatrices.push_back(child->m_modelMatrix);

            std::vector<glm::mat4> childrenMatrices;
            auto childrenObjects = child->getSceneNodesFlat(childrenMatrices);

            temp.insert(temp.end(), childrenObjects.begin(), childrenObjects.end());
            modelMatrices.insert(modelMatrices.end(), childrenMatrices.begin(), childrenMatrices.end());
        }

        return temp;
    }

private:
    /* world space model matrix */
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    bool m_modelMatrixChanged = true;

    Transform m_localTransform;
    bool m_localTransformDirty = true;

    std::vector<T *> m_children;

    SceneNode<T> *m_parent = nullptr;

    bool modelMatrixChanged() { return m_modelMatrixChanged; }
};

}  // namespace vengine

#endif