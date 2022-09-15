#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/VulkanDynamicUBO.hpp>
#include <vulkan/VulkanDataStructs.hpp>
#include <vulkan/VulkanMesh.hpp>

class VulkanSceneObject : public SceneObject
{
public:
    VulkanSceneObject(const Mesh * meshModel, VulkanDynamicUBO<ModelData>& transformDynamicUBO);

    void setModelMatrix(const glm::mat4& modelMatrix) override;

    void updateModelMatrixData(const glm::mat4& modelMatrix);

    uint32_t getTransformUBOBlock() const;

private:
    ModelData* m_modelData = nullptr;
    uint32_t m_transformUBOBlock = -1;
};

#endif