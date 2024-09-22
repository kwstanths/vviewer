#ifndef __VulkanScene_hpp__
#define __VulkanScene_hpp__

#include <memory>

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Light.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/VulkanInstances.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanUBOAccessors.hpp"

namespace vengine
{

class VulkanEngine;

class VulkanScene : public Scene
{
    friend class VulkanRenderer;

public:
    VulkanScene(VulkanContext &vkctx, VulkanEngine& engine);
    ~VulkanScene();

    VkResult initResources();
    VkResult initSwapchainResources(uint32_t nImages);

    VkResult releaseResources();
    VkResult releaseSwapchainResources();

    VkDescriptorSetLayout &descriptorSetlayoutSceneData() { return m_descriptorSetLayoutScene; }
    VkDescriptorSetLayout &descriptorSetlayoutInstanceData() { return m_instances.layoutInstanceData(); }
    VkDescriptorSetLayout &descriptorSetlayoutLights() { return m_descriptorSetLayoutLight; }
    VkDescriptorSetLayout &descriptorSetlayoutTLAS() { return m_descriptorSetLayoutTLAS; }

    VkDescriptorSet &descriptorSetSceneData(uint32_t imageIndex) { return m_descriptorSetsScene[imageIndex]; };
    VkDescriptorSet &descriptorSetInstanceData(uint32_t imageIndex) { return m_instances.descriptorSetInstanceData(imageIndex); };
    VkDescriptorSet &descriptorSetLight(uint32_t imageIndex) { return m_descriptorSetsLight[imageIndex]; };
    VkDescriptorSet &descriptorSetTLAS(uint32_t imageIndex) { return m_descriptorSetsTLAS[imageIndex]; };

    SceneData getSceneData() const override;

    void updateFrame(VulkanCommandInfo vci, uint32_t frameIndex);

    Light *createLight(const AssetInfo &info, LightType type, glm::vec4 color = {1, 1, 1, 1}) override;

    InstancesManager &instancesManager() override { return m_instances; }

private:
    VulkanContext &m_vkctx;

    VulkanInstancesManager m_instances;

    VkDescriptorPool m_descriptorPool;

    /* Buffers to hold the scene data struct */
    std::vector<VulkanBuffer> m_uniformBuffersScene;
    /* SceneData descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;

    /* Buffers to hold light data */
    VulkanUBODefault<LightData> m_lightDataUBO;
    /* Lights descriptor sets */
    VkDescriptorSetLayout m_descriptorSetLayoutLight;
    std::vector<VkDescriptorSet> m_descriptorSetsLight;

    /* Data for TLAS */
    struct BLASInstance {
        const VulkanAccelerationStructure &accelerationStructure;
        const glm::mat4 &modelMatrix;
        uint32_t SBTOffset; /* SBT Ioffset */
        uint32_t instanceDataOffset;

        BLASInstance(const VulkanAccelerationStructure &_accelerationStructure,
                     const glm::mat4 &_modelMatrix,
                     uint32_t _SBTOffset,
                     uint32_t _instanceDataOffset)
            : accelerationStructure(_accelerationStructure)
            , modelMatrix(_modelMatrix)
            , SBTOffset(_SBTOffset)
            , instanceDataOffset(_instanceDataOffset){};
    };
    std::vector<VulkanAccelerationStructure> m_tlas;
    VkDescriptorSetLayout m_descriptorSetLayoutTLAS;
    std::vector<VkDescriptorSet> m_descriptorSetsTLAS;
    std::vector<bool> m_tlasNeedsUpdate;

    VkResult createDescriptorSetsLayouts();
    VkResult createDescriptorPool(uint32_t nImages);
    VkResult createDescriptorSets(uint32_t nImages);
    VkResult createBuffers(uint32_t nImages);

    SceneObject *createObject(std::string name) override;
    void deleteObject(SceneObject *object) override;
    void invalidateTLAS() override;

    void buildTLAS(VulkanCommandInfo vci, uint32_t imageIndex);
    void createTopLevelAccelerationStructure(const std::vector<BLASInstance> &blasInstances,
                                             VulkanCommandInfo vci,
                                             uint32_t imageIndex);
};

}  // namespace vengine

#endif  // !__VulkanScene_hpp__
