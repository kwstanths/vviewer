#ifndef __VulkanRendererBase_hpp__
#define __VulkanRendererBase_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanSceneObject.hpp"

namespace vengine
{
class VulkanRendererBase
{
public:
    VulkanRendererBase();

    virtual VkResult renderObjectsForwardOpaque(VkCommandBuffer &cmdBuf,
                                                VkDescriptorSet &descriptorScene,
                                                VkDescriptorSet &descriptorModel,
                                                VkDescriptorSet &descriptorLight,
                                                VkDescriptorSet descriptorSkybox,
                                                VkDescriptorSet &descriptorMaterial,
                                                VkDescriptorSet &descriptorTextures,
                                                VkDescriptorSet &descriptorTLAS,
                                                const SceneGraph &objects,
                                                const SceneGraph &lights) const = 0;

    virtual VkResult renderObjectsForwardTransparent(VkCommandBuffer &cmdBuf,
                                                     VkDescriptorSet &descriptorScene,
                                                     VkDescriptorSet &descriptorModel,
                                                     VkDescriptorSet &descriptorLight,
                                                     VkDescriptorSet descriptorSkybox,
                                                     VkDescriptorSet &descriptorMaterials,
                                                     VkDescriptorSet &descriptorTextures,
                                                     VkDescriptorSet &descriptorTLAS,
                                                     SceneObject *object,
                                                     const SceneGraph &lights) const = 0;

protected:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_queue;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutLight,
        m_descriptorSetLayoutSkybox, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures, m_descriptorSetLayoutTLAS;

    VkRenderPass m_renderPass;
    VkSampleCountFlagBits m_msaaSamples;

    VkResult initResources(VkPhysicalDevice physicalDevice,
                           VkDevice device,
                           VkQueue queue,
                           VkCommandPool commandPool,
                           VkPhysicalDeviceProperties physicalDeviceProperties,
                           VkDescriptorSetLayout cameraDescriptorLayout,
                           VkDescriptorSetLayout modelDescriptorLayout,
                           VkDescriptorSetLayout lightDescriptorLayout,
                           VkDescriptorSetLayout skyboxDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VkDescriptorSetLayout texturesDescriptorLayout,
                           VkDescriptorSetLayout tlasDescriptorLayout);

    VkResult initSwapChainResources(VkExtent2D swapchainExtent,
                                    VkRenderPass renderPass,
                                    uint32_t swapchainImages,
                                    VkSampleCountFlagBits msaaSamples);

    VkResult createPipelineForwardOpaque(VkShaderModule vertexShader,
                                         VkShaderModule fragmentShader,
                                         VkPipelineLayout &pipelineLayout,
                                         VkPipeline &pipeline);
    VkResult createPipelineForwardTransparent(VkShaderModule vertexShader,
                                              VkShaderModule fragmentShader,
                                              VkPipelineLayout &pipelineLayout,
                                              VkPipeline &pipeline);

    VkResult renderObjects(VkCommandBuffer &commandBuffer,
                           const VkPipelineLayout &pipelineLayout,
                           const SceneGraph &objects,
                           const SceneGraph &lights) const;

private:
};

}  // namespace vengine

#endif