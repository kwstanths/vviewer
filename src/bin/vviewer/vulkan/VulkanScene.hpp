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

    void setCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getCamera() const;

    void setSkybox(VulkanMaterialSkybox* skybox);
    VulkanMaterialSkybox* getSkybox() const;

    void setDirectionalLight(std::shared_ptr<DirectionalLight> directionaLight);
    std::shared_ptr<DirectionalLight> getDirectionalLight() const;

    VulkanSceneObject* addObject(MeshModel* meshModel, Transform& transform);

    /* Flush buffer changes to gpu */
    void updateBuffers(VkDevice device, uint32_t imageIndex) const;

private:
    VulkanMaterialSkybox* m_skybox = nullptr;
    std::vector<VulkanSceneObject*> m_objects;
    std::shared_ptr<Camera> m_camera;
    std::shared_ptr<DirectionalLight> m_directionalLight;

    /* Dynamic uniform buffer to hold model positions */
    VulkanDynamicUBO<ModelData> m_modelDataDynamicUBO;
    size_t m_transformIndexUBO = 0;
    /* Buffers to hold the scene data */
    std::vector<VkBuffer> m_uniformBuffersScene;
    std::vector<VkDeviceMemory> m_uniformBuffersSceneMemory;
};

#endif // !__VulkanScene_hpp__
