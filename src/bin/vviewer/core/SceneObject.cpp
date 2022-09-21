#include "SceneObject.hpp"

#include "vulkan/VulkanSceneObject.hpp"

SceneObject::SceneObject(const Transform& t) : SceneNode(t), Entity()
{
    m_idRGB = IDGeneration::toRGB(getID());
}

SceneObject::~SceneObject()
{
    if (has(ComponentType::POINT_LIGHT)){
        delete get(ComponentType::POINT_LIGHT);
    }
}

void SceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
}

glm::vec3 SceneObject::getIDRGB() const
{
    return m_idRGB;
}
