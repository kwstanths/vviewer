#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/common/VulkanStructs.hpp>
#include <vulkan/resources/VulkanUBOAccessors.hpp>
#include <vulkan/resources/VulkanMesh.hpp>

namespace vengine
{

class VulkanSceneObject : public SceneObject
{
public:
    VulkanSceneObject(Scene *scene, const std::string &name);

    ~VulkanSceneObject();

private:
};

}  // namespace vengine

#endif