#ifndef __VulkanRendererPBR_hpp__
#define __VulkanRendererPBR_hpp__

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTextures.hpp"
#include "vulkan/renderers/VulkanRendererBase.hpp"

namespace vengine
{

class VulkanRendererPBR : public VulkanRendererBase
{
    friend class VulkanRenderer;

public:
    VulkanRendererPBR();

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
                           VulkanTextures &textures,
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
                                        VkDescriptorSet &descriptorMaterials,
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

    VkResult createBRDFLUT(VulkanTextures &textures, uint32_t resolution = 128) const;
};

}  // namespace vengine

#endif