#ifndef __Node_hpp__
#define __Node_hpp__

#include <vector>
#include <memory>

#include <vengine/math/Transform.hpp>

namespace vengine
{

template <typename T>
class SceneNode
{
public:
    SceneNode(Transform transform)
        : m_localTransform(transform){};

    const Transform &localTransform() const { return m_localTransform; }
    Transform &localTransform() { return m_localTransform; }

    const glm::mat4 &modelMatrix() const { return m_modelMatrix; }
    glm::mat4 &modelMatrix() { return m_modelMatrix; }

    glm::vec3 worldPosition() const;

    const SceneNode<T> *parent() const { return m_parent; }
    SceneNode<T> *&parent() { return m_parent; }

    const std::vector<T *> &children() const { return m_children; }

    T *addChild(T *node);

    void removeChild(T *node);

    void update();

    /* Get all nodes in a flat array */
    std::vector<T *> getSceneObjectsArray();
    std::vector<T *> getSceneObjectsArray(std::vector<glm::mat4> &modelMatrices);

    virtual void setModelMatrix(const glm::mat4 &modelMatrix) = 0;

private:
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    Transform m_localTransform;

    std::vector<T *> m_children;

    SceneNode<T> *m_parent = nullptr;
};

}  // namespace vengine

#endif