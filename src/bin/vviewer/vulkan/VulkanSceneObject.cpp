#include "VulkanSceneObject.hpp"

VulkanSceneObject::VulkanSceneObject(VulkanDynamicUBO<ModelData>& transformDynamicUBO)
    : SceneObject(Transform())
{
    /* Get a free block index to store the model matrix */
    m_transformUBOBlock = static_cast<uint32_t>(transformDynamicUBO.getFree());
    m_modelData = transformDynamicUBO.getBlock(m_transformUBOBlock);

    updateModelMatrixData(glm::mat4(1.0f));
}

void VulkanSceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
    updateModelMatrixData(modelMatrix);
}

void VulkanSceneObject::updateModelMatrixData(const glm::mat4& modelMatrix)
{
    m_modelData->m_modelMatrix = modelMatrix;
}

uint32_t VulkanSceneObject::getTransformUBOBlock() const
{
    return m_transformUBOBlock;
}

