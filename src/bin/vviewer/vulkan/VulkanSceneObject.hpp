#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/VulkanDynamicUBO.hpp>
#include <vulkan/VulkanDataStructs.hpp>
#include <vulkan/VulkanMesh.hpp>

class VulkanSceneObject : public SceneObject
{
public:
    VulkanSceneObject(const MeshModel * meshModel, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock);

    void setModelMatrix(const glm::mat4& modelMatrix) override;

    void updateModelMatrixData(const glm::mat4& modelMatrix);

    int getTransformUBOBlock() const;

private:
    VulkanDynamicUBO<ModelData>& m_transformDataSotrage;
    int m_transformUBOBlock = -1;
};

#endif