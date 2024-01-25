#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/common/VulkanStructs.hpp>
#include <vulkan/resources/VulkanUBO.hpp>
#include <vulkan/resources/VulkanMesh.hpp>

namespace vengine
{

class VulkanSceneObject : public SceneObject, private VulkanUBOBlock<ModelData>
{
public:
    VulkanSceneObject(const std::string &name, VulkanUBO<ModelData> &transformUBO);

    ~VulkanSceneObject();

    void setModelMatrix(const glm::mat4 &modelMatrix) override;

    void updateModelMatrixData(const glm::mat4 &modelMatrix);

    uint32_t getModelDataUBOIndex() const;

private:
};

}  // namespace vengine

#endif