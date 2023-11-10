#ifndef __VulkanScene_hpp__
#define __VulkanScene_hpp__

#include <memory>

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Light.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanUBO.hpp"

namespace vengine
{

class VulkanScene : public Scene
{
    friend class VulkanRenderer;

public:
    VulkanScene(VulkanContext &vkctx);
    ~VulkanScene();

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &layoutSceneData() { return m_descriptorSetLayoutScene; }
    VkDescriptorSetLayout &layoutModelData() { return m_descriptorSetLayoutModel; }
    VkDescriptorSetLayout &layoutLights() { return m_descriptorSetLayoutLight; }

    VkDescriptorSet &descriptorSetSceneData(uint32_t imageIndex) { return m_descriptorSetsScene[imageIndex]; };
    VkDescriptorSet &descriptorSetModelData(uint32_t imageIndex) { return m_descriptorSetsModel[imageIndex]; };
    VkDescriptorSet &descriptorSetLight(uint32_t imageIndex) { return m_descriptorSetsLight[imageIndex]; };

    virtual SceneData getSceneData() const override;

    /* Flush buffer changes to gpu */
    void updateBuffers(const std::vector<SceneObject *> &lights, uint32_t imageIndex) const;

    Light *createLight(const std::string &name, LightType type, glm::vec4 color = {1, 1, 1, 1}) override;

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;

    /* Buffers to hold the scene data struct */
    std::vector<VulkanBuffer> m_uniformBuffersScene;
    /* SceneData descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;

    /* Buffers to hold model matrices */
    VulkanUBO<ModelData> m_modelDataUBO;
    /* ModelData descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    /* Buffers to hold light data */
    VulkanUBO<LightData> m_lightDataUBO;
    /* Buffers to hold light components */
    VulkanUBO<LightComponent> m_lightComponentsUBO;
    /* Lights descriptor sets */
    VkDescriptorSetLayout m_descriptorSetLayoutLight;
    std::vector<VkDescriptorSet> m_descriptorSetsLight;

    VkResult createDescriptorSetsLayouts();
    VkResult createDescriptorPool(uint32_t nImages);
    VkResult createDescriptorSets(uint32_t nImages);
    VkResult createBuffers(uint32_t nImages);

    SceneObject *createObject(std::string name) override;
    void deleteObject(SceneObject *object) override;
};

}  // namespace vengine

#endif  // !__VulkanScene_hpp__
