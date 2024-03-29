#ifndef __VulkanScene_hpp__
#define __VulkanScene_hpp__

#include <memory>

#include "core/Scene.hpp"
#include "core/Camera.hpp"
#include "core/Light.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanContext.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanUBOAccessors.hpp"

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
    VkDescriptorSetLayout &layoutTLAS() { return m_descriptorSetLayoutTLAS; }

    VkDescriptorSet &descriptorSetSceneData(uint32_t imageIndex) { return m_descriptorSetsScene[imageIndex]; };
    VkDescriptorSet &descriptorSetModelData(uint32_t imageIndex) { return m_descriptorSetsModel[imageIndex]; };
    VkDescriptorSet &descriptorSetLight(uint32_t imageIndex) { return m_descriptorSetsLight[imageIndex]; };
    VkDescriptorSet &descriptorSetTLAS(uint32_t imageIndex) { return m_descriptorSetsTLAS[imageIndex]; };

    SceneData getSceneData() const override;

    void updateBuffers(const std::vector<SceneObject *> &meshes,
                       const std::vector<SceneObject *> &lights,
                       const std::vector<std::pair<SceneObject *, uint32_t>> &meshLights,
                       uint32_t imageIndex);
    void updateTLAS(VulkanCommandInfo vci, const std::vector<SceneObject *> &meshes, uint32_t imageIndex);

    Light *createLight(const AssetInfo &info, LightType type, glm::vec4 color = {1, 1, 1, 1}) override;

private:
    VulkanContext &m_vkctx;

    VkDescriptorPool m_descriptorPool;

    /* Buffers to hold the scene data struct */
    std::vector<VulkanBuffer> m_uniformBuffersScene;
    /* SceneData descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutScene;
    std::vector<VkDescriptorSet> m_descriptorSetsScene;

    /* Buffers to hold model matrices */
    VulkanUBOCached<ModelData> m_modelDataUBO;
    /* ModelData descriptor */
    VkDescriptorSetLayout m_descriptorSetLayoutModel;
    std::vector<VkDescriptorSet> m_descriptorSetsModel;

    /* Buffers to hold light data */
    VulkanUBODefault<LightData> m_lightDataUBO;
    /* Buffers to hold light instances */
    VulkanUBODefault<LightInstance> m_lightInstancesUBO;
    /* Lights descriptor sets */
    VkDescriptorSetLayout m_descriptorSetLayoutLight;
    std::vector<VkDescriptorSet> m_descriptorSetsLight;

    /* Data for TLAS and scene object description */
    struct BLASInstance {
        const VulkanAccelerationStructure &accelerationStructure;
        const glm::mat4 &modelMatrix;
        uint32_t instanceOffset; /* SBT Ioffset */
        BLASInstance(const VulkanAccelerationStructure &_accelerationStructure,
                     const glm::mat4 &_modelMatrix,
                     uint32_t _instanceOffset)
            : accelerationStructure(_accelerationStructure)
            , modelMatrix(_modelMatrix)
            , instanceOffset(_instanceOffset){};
    };
    std::vector<BLASInstance> m_blasInstances;
    std::vector<ObjectDescriptionPT> m_sceneObjectsDescription;
    std::vector<VulkanAccelerationStructure> m_tlas;
    std::vector<VulkanBuffer> m_storageBufferObjectDescription;
    VkDescriptorSetLayout m_descriptorSetLayoutTLAS;
    std::vector<VkDescriptorSet> m_descriptorSetsTLAS;

    VkResult createDescriptorSetsLayouts();
    VkResult createDescriptorPool(uint32_t nImages);
    VkResult createDescriptorSets(uint32_t nImages);
    VkResult createBuffers(uint32_t nImages);

    SceneObject *createObject(std::string name) override;
    void deleteObject(SceneObject *object) override;

    void createTopLevelAccelerationStructure(VulkanCommandInfo vci, uint32_t imageIndex);
};

}  // namespace vengine

#endif  // !__VulkanScene_hpp__
