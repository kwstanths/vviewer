#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/common/VulkanStructs.hpp>
#include <vulkan/resources/VulkanUBOAccessors.hpp>
#include <vulkan/resources/VulkanMesh.hpp>

namespace vengine
{

class VulkanSceneObject : public SceneObject, protected VulkanUBOCached<ModelData>::Block
{
public:
    VulkanSceneObject(const std::string &name, VulkanUBOCached<ModelData> &transformUBO);

    ~VulkanSceneObject();

    void setModelMatrix(const glm::mat4 &modelMatrix) override;

    void updateModelMatrixData(const glm::mat4 &modelMatrix);

    uint32_t getModelDataUBOIndex() const;

private:
};

}  // namespace vengine

#endif