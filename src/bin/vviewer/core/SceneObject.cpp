#include "SceneObject.hpp"

SceneObject::SceneObject(const MeshModel * meshModel, Transform transform)
{
    m_meshModel = meshModel;
    m_transform = transform;
}

Transform SceneObject::getTransform()
{
    return m_transform;
}

void SceneObject::setTransform(const Transform & transform)
{
    m_transform = transform;
}

const MeshModel * SceneObject::getMeshModel()
{
    return m_meshModel;
}
