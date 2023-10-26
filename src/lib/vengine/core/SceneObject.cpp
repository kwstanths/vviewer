#include "SceneObject.hpp"

#include "vulkan/VulkanSceneObject.hpp"

namespace vengine {

SceneObject::SceneObject(const Transform& t) : SceneNode(t), Entity()
{
    m_idRGB = vengine::IDGeneration::toRGB(getID());
}

SceneObject::~SceneObject()
{
}

void SceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
}

glm::vec3 SceneObject::getIDRGB() const
{
    return m_idRGB;
}

}