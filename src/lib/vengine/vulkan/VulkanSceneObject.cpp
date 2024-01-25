#include "VulkanSceneObject.hpp"

#include <iostream>

namespace vengine
{

VulkanSceneObject::VulkanSceneObject(const std::string &name, VulkanUBO<ModelData> &transformUBO)
    : SceneObject(name, Transform())
    , VulkanUBOBlock<ModelData>(transformUBO)
{
    updateModelMatrixData(glm::mat4(1.0f));
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
    m_block->m_modelMatrix = modelMatrix;
}

uint32_t VulkanSceneObject::getModelDataUBOIndex() const
{
    return UBOBlockIndex();
}

}  // namespace vengine
