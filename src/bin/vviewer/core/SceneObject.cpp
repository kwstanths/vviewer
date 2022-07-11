#include "SceneObject.hpp"

#include "vulkan/VulkanSceneObject.hpp"

SceneObject::SceneObject(const Transform& t) : SceneNode(t)
{
    m_id = IDGeneration::getInstance().getID();
    m_idRGB = IDGeneration::toRGB(m_id);
}

SceneObject::SceneObject(const Transform& t, const Mesh* mesh) : SceneObject(t)
{
    m_mesh = mesh;
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

ID SceneObject::getID() const
{
    return m_id;
}

glm::vec3 SceneObject::getIDRGB() const
{
    return m_idRGB;
}
