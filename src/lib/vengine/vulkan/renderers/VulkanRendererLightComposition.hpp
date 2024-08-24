#ifndef __VulkanRendererLightComposition_hpp__
#define __VulkanRendererLightComposition_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanInstances.hpp"
#include "vulkan/VulkanRenderPass.hpp"

namespace vengine
{
class VulkanRendererLightComposition
{
public:
    VulkanRendererLightComposition(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout cameraDescriptorLayout,
                           VkDescriptorSetLayout instanceDataDescriptorLayout,
                           VkDescriptorSetLayout lightDataDescriptorLayout,
                           VkDescriptorSetLayout skyboxDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VkDescriptorSetLayout texturesDescriptorLayout,
                           VkDescriptorSetLayout tlasDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassDeferred &renderPass);

    VkResult releaseResources();
    VkResult releaseSwapChainResources();

    VkResult renderIBL(VkCommandBuffer &commandBuffer,
                       const InstancesManager &instances,
                       VkDescriptorSet &descriptorGBuffer,
                       VkDescriptorSet &descriptorScene,
                       VkDescriptorSet &descriptorModel,
                       VkDescriptorSet &descriptorLight,
                       VkDescriptorSet descriptorSkybox,
                       VkDescriptorSet &descriptorMaterials,
                       VkDescriptorSet &descriptorTextures,
                       VkDescriptorSet &descriptorTLAS);
    VkResult renderLights(VkCommandBuffer &commandBuffer,
                          const InstancesManager &instances,
                          VkDescriptorSet &descriptorGBuffer,
                          VkDescriptorSet &descriptorScene,
                          VkDescriptorSet &descriptorModel,
                          VkDescriptorSet &descriptorLight,
                          VkDescriptorSet descriptorSkybox,
                          VkDescriptorSet &descriptorMaterials,
                          VkDescriptorSet &descriptorTextures,
                          VkDescriptorSet &descriptorTLAS) const;

private:
    VulkanContext &m_ctx;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera, m_descriptorSetLayoutInstanceData, m_descriptorSetLayoutLightData,
        m_descriptorSetLayoutSkybox, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures, m_descriptorSetLayoutTLAS;

    VkPipelineLayout m_pipelineLayoutIBL, m_pipelineLayoutPoint, m_pipelineLayoutDirectional;
    VkPipeline m_graphicsPipelineIBL, m_graphicsPipelinePoint, m_graphicsPipelineDirectional;

    /* Pipelines creation */
    VkResult createPipelineIBL(const VulkanRenderPassDeferred &renderPass);
    VkResult createPipelineLightInstance(const VulkanRenderPassDeferred &renderPass,
                                         std::string fragmentShader,
                                         VkPipelineLayout &outPipelineLayout,
                                         VkPipeline &outGraphicsPipeline);
};

}  // namespace vengine

#endif