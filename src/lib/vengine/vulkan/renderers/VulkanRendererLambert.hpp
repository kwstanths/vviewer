#ifndef __VulkanRendererLambert_hpp__
#define __VulkanRendererLambert_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/renderers/VulkanRendererBase.hpp"

namespace vengine
{

class VulkanRendererLambert : public VulkanRendererBase
{
    friend class VulkanRenderer;

public:
    VulkanRendererLambert();

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

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    VkResult renderObjectsForwardOpaque(VkCommandBuffer &cmdBuf,
                                        const VulkanInstancesManager &instances,
                                        VkDescriptorSet &descriptorScene,
                                        VkDescriptorSet &descriptorModel,
                                        VkDescriptorSet &descriptorLight,
                                        VkDescriptorSet descriptorSkybox,
                                        VkDescriptorSet &descriptorMaterial,
                                        VkDescriptorSet &descriptorTextures,
                                        VkDescriptorSet &descriptorTLAS,
                                        const SceneGraph &lights) const override;

    VkResult renderObjectsForwardTransparent(VkCommandBuffer &cmdBuf,
                                             VulkanInstancesManager &instances,
                                             VkDescriptorSet &descriptorScene,
                                             VkDescriptorSet &descriptorModel,
                                             VkDescriptorSet &descriptorLight,
                                             VkDescriptorSet descriptorSkybox,
                                             VkDescriptorSet &descriptorMaterials,
                                             VkDescriptorSet &descriptorTextures,
                                             VkDescriptorSet &descriptorTLAS,
                                             SceneObject *object,
                                             const SceneGraph &lights) const override;

private:
    VkPipelineLayout m_pipelineLayoutForwardOpaque, m_pipelineLayoutForwardTransparent;
    VkPipeline m_graphicsPipelineForwardOpaque, m_graphicsPipelineForwardTransparent;
};

}  // namespace vengine

#endif