#include "VulkanSceneObject.hpp"

VulkanSceneObject::VulkanSceneObject(const MeshModel * meshModel, Transform transform, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock)
    : SceneObject(meshModel, transform)
{
    m_transformUBOBlock = transformUBOBlock;
    updateModelMatrixData(transformDynamicUBO);
}

void VulkanSceneObject::updateModelMatrixData(VulkanDynamicUBO<ModelData>& transformDynamicUBO)
{
    ModelData * modelData = transformDynamicUBO.getBlock(m_transformUBOBlock);
    modelData->m_modelMatrix = m_transform.getModelMatrix();
}

int VulkanSceneObject::getTransformUBOBlock() const
{
    return m_transformUBOBlock;
}

