#include "VulkanSceneObject.hpp"

#include <iostream>

namespace vengine
{

VulkanSceneObject::VulkanSceneObject(VulkanUBO<ModelData> &transformDynamicUBO)
    : SceneObject(Transform())
    , VulkanUBOBlock<ModelData>(transformDynamicUBO)
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

}  // namespace vengine
