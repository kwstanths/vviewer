#include "VulkanSceneObject.hpp"

#include <iostream>

namespace vengine
{

VulkanSceneObject::VulkanSceneObject(const std::string &name, VulkanUBOCached<ModelData> &transformUBO)
    : SceneObject(name, Transform())
    , VulkanUBOCached<ModelData>::Block(transformUBO, {glm::mat4(1.0f)})
{
}

VulkanSceneObject::~VulkanSceneObject()
{
}

void VulkanSceneObject::setModelMatrix(const glm::mat4 &modelMatrix)
{
    updateModelMatrixData(modelMatrix);
}

void VulkanSceneObject::updateModelMatrixData(const glm::mat4 &modelMatrix)
{
    VulkanUBOCached<ModelData>::Block::updateValue({modelMatrix});
}

uint32_t VulkanSceneObject::getModelDataUBOIndex() const
{
    return UBOBlockIndex();
}

}  // namespace vengine
