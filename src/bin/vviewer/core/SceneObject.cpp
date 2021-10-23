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

const MeshModel * SceneObject::getMeshModel() const
{
    return m_meshModel;
}

void SceneObject::setMeshModel(const MeshModel * newMeshModel)
{
    m_meshModel = newMeshModel;
}

Material * SceneObject::getMaterial() const
{
    return m_material;
}

void SceneObject::setMaterial(Material * newMaterial)
{
    m_material = newMaterial;
}
