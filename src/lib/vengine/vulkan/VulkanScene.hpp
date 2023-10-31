#ifndef __VulkanScene_hpp__
#define __VulkanScene_hpp__

#include <memory>

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Lights.hpp"

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
    VulkanScene(VulkanContext &vkctx, uint32_t maxObjects);
    ~VulkanScene();

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &layoutSceneData() { return m_descriptorSetLayoutScene; }
    VkDescriptorSetLayout &layoutModelData() { return m_descriptorSetLayoutModel; }

    VkDescriptorSet &descriptorSetSceneData(uint32_t imageIndex) { return m_descriptorSetsScene[imageIndex]; };
    VkDescriptorSet &descriptorSetModelData(uint32_t imageIndex) { return m_descriptorSetsModel[imageIndex]; };

    virtual SceneData getSceneData() const override;

    /* Flush buffer changes to gpu */
    void updateBuffers(uint32_t imageIndex) const;

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;

    /* Buffers to hold the scene data struct */
    std::vector<VulkanBuffer> m_uniformBuffersScene;
    /* Scene data struct descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;

    /* Dynamic uniform buffer to hold model matrices */
    VulkanUBO<ModelData> m_modelDataDynamicUBO;
    /* Model data descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    VkResult createDescriptorSetsLayouts();
    VkResult createDescriptorPool(uint32_t nImages);
    VkResult createDescriptorSets(uint32_t nImages);
    VkResult createBuffers(uint32_t nImages);

    SceneObject *createObject(std::string name) override;
    void deleteObject(SceneObject *object) override;
};

}  // namespace vengine

#endif  // !__VulkanScene_hpp__
