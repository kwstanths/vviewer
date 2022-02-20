#include "SceneObject.hpp"

SceneObject::SceneObject(const Mesh * mesh)
{
    m_mesh= mesh;
}

void SceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
    
}

const Mesh * SceneObject::getMesh() const
{
    return m_mesh;
}

void SceneObject::setMesh(const Mesh * newMesh)
{
    m_mesh = newMesh;
}

Material * SceneObject::getMaterial() const
{
    return m_material;
}

void SceneObject::setMaterial(Material * newMaterial)
{
    m_material = newMaterial;
}
