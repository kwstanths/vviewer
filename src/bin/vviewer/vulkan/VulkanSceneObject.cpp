#include "VulkanSceneObject.hpp"

VulkanSceneObject::VulkanSceneObject(const MeshModel * meshModel, Transform transform, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock)
    : SceneObject(meshModel, transform), m_transformDataSotrage(transformDynamicUBO)
{
    m_transformUBOBlock = transformUBOBlock;
    updateModelMatrixData();
}

void VulkanSceneObject::setTransform(const Transform & transform)
{
    m_transform = transform;
    updateModelMatrixData();
}

void VulkanSceneObject::updateModelMatrixData()
{
    ModelData * modelData = m_transformDataSotrage.getBlock(m_transformUBOBlock);
    modelData->m_modelMatrix = m_transform.getModelMatrix();
}

int VulkanSceneObject::getTransformUBOBlock() const
{
    return m_transformUBOBlock;
}

