#include "VulkanSceneObject.hpp"

#include <iostream>

namespace vengine
{

VulkanSceneObject::VulkanSceneObject(Scene *scene, const std::string &name)
    : SceneObject(scene, name, Transform())
{
}

VulkanSceneObject::~VulkanSceneObject()
{
}

}  // namespace vengine
