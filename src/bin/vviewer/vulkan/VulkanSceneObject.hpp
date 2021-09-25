#ifndef __VulkanSceneObject_hpp__
#define __VulkanSceneObject_hpp__

#include <core/SceneObject.hpp>
#include <vulkan/VulkanDynamicUBO.hpp>
#include <vulkan/VulkanDataStructs.hpp>
#include <vulkan/VulkanMesh.hpp>

class VulkanSceneObject : public SceneObject
{
public:
    VulkanSceneObject(const MeshModel * meshModel, Transform transform, VulkanDynamicUBO<ModelData>& transformDynamicUBO, int transformUBOBlock);

    void setTransform(const Transform& transform) override;

    void updateModelMatrixData();

    int getTransformUBOBlock() const;

private:
    VulkanDynamicUBO<ModelData>& m_transformDataSotrage;
    int m_transformUBOBlock = -1;
};

#endif