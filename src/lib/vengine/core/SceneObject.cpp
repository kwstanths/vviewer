#include "SceneObject.hpp"
#include "Scene.hpp"

#include "vulkan/VulkanSceneObject.hpp"

namespace vengine
{

SceneObject::SceneObject(Scene *scene, const std::string &name, const Transform &t)
    : SceneNode(t)
    , Entity()
    , m_name(name)
    , m_scene(scene)
{
    m_idRGB = vengine::IDGeneration::toRGB(getID());
}

SceneObject::~SceneObject()
{
}

glm::vec3 SceneObject::getIDRGB() const
{
    return m_idRGB;
}

void SceneObject::setModelMatrix(const glm::mat4 &modelMatrix)
{
}

void SceneObject::transformChanged()
{
    m_scene->needsUpdate(true);
}

}  // namespace vengine