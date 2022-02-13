#include "SceneObject.hpp"

SceneObject::SceneObject(const MeshModel * meshModel)
{
    m_meshModel = meshModel;
}

void SceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
    
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
