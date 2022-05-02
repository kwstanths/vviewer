#ifndef __VulkanScene_hpp__
#define __VulkanScene_hpp__

#include <memory>

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Lights.hpp"

#include "VulkanMaterials.hpp"
#include "VulkanSceneObject.hpp"
#include "VulkanDataStructs.hpp"
#include "VulkanDynamicUBO.hpp"

class VulkanScene : public Scene {
    friend class VulkanRenderer;
public:
    VulkanScene();
    ~VulkanScene();

    void setSkybox(VulkanMaterialSkybox* skybox);
    VulkanMaterialSkybox* getSkybox() const;

    virtual SceneData getSceneData() const override;

    /* Add a new scene object at the root of the scene graph */
    std::shared_ptr<SceneNode> addSceneObject(std::string meshModel, Transform transform, std::string material) override;
    /* Add a new scene object as a child of a node */
    std::shared_ptr<SceneNode> addSceneObject(std::shared_ptr<SceneNode> node, std::string meshModel, Transform transform, std::string material) override;

    /* Flush buffer changes to gpu */
    void updateBuffers(VkDevice device, uint32_t imageIndex) const;

private:
    VulkanMaterialSkybox* m_skybox = nullptr;

    /* Dynamic uniform buffer to hold model positions */
    VulkanDynamicUBO<ModelData> m_modelDataDynamicUBO;
    size_t m_transformIndexUBO = 0;
    /* Buffers to hold the scene data */
    std::vector<VkBuffer> m_uniformBuffersScene;
    std::vector<VkDeviceMemory> m_uniformBuffersSceneMemory;

    std::vector<std::shared_ptr<VulkanSceneObject>> createObject(std::string meshModel, std::string material);
};

#endif // !__VulkanScene_hpp__
