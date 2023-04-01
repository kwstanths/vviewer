#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/VulkanStructs.hpp>
#include <vulkan/resources/VulkanUBO.hpp>
#include <vulkan/resources/VulkanMesh.hpp>

class VulkanSceneObject : public SceneObject
{
public:
    VulkanSceneObject(VulkanUBO<ModelData>& transformDynamicUBO);

    ~VulkanSceneObject();

    void setModelMatrix(const glm::mat4& modelMatrix) override;

    void updateModelMatrixData(const glm::mat4& modelMatrix);

    uint32_t getTransformUBOBlock() const;

private:
    ModelData* m_modelData = nullptr;
    uint32_t m_transformUBOBlock = -1;
    VulkanUBO<ModelData>& m_transformDynamicUBO;
};

#endif