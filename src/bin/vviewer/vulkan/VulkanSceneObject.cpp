#include "VulkanSceneObject.hpp"

VulkanSceneObject::VulkanSceneObject(const Mesh * mesh, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock)
    : SceneObject(Transform())
{
    m_mesh = mesh;
    m_transformUBOBlock = transformUBOBlock;
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

int VulkanSceneObject::getTransformUBOBlock() const
{
    return m_transformUBOBlock;
}

