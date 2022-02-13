#include "VulkanSceneObject.hpp"

VulkanSceneObject::VulkanSceneObject(const MeshModel * meshModel, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock)
    : SceneObject(meshModel), m_transformDataSotrage(transformDynamicUBO)
{
    m_transformUBOBlock = transformUBOBlock;
    updateModelMatrixData(glm::mat4(1.0f));
}

void VulkanSceneObject::setModelMatrix(const glm::mat4& modelMatrix)
{
    updateModelMatrixData(modelMatrix);
}

void VulkanSceneObject::updateModelMatrixData(const glm::mat4& modelMatrix)
{
    ModelData * modelData = m_transformDataSotrage.getBlock(m_transformUBOBlock);
    modelData->m_modelMatrix = modelMatrix;
}

int VulkanSceneObject::getTransformUBOBlock() const
{
    return m_transformUBOBlock;
}

