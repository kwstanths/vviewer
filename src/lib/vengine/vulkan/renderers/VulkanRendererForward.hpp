#ifndef __VulkanRendererForward_hpp__
#define __VulkanRendererForward_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanInstances.hpp"
#include "vulkan/VulkanRenderPass.hpp"

namespace vengine
{
class VulkanRendererForward
{
public:
    VulkanRendererForward(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout sceneDataDescriptorLayout,
                           VkDescriptorSetLayout instanceDataDescriptorLayout,
                           VkDescriptorSetLayout lightDescriptorLayout,
                           VkDescriptorSetLayout skyboxDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VkDescriptorSetLayout texturesDescriptorLayout,
                           VkDescriptorSetLayout tlasDescriptorLayout);

    VkResult initSwapChainResources(VkExtent2D swapchainExtent,
                                    const VulkanRenderPassDeferred &renderPass,
                                    const std::string &fragmentShader);

    VkResult releaseResources();
    VkResult releaseSwapChainResources();

    const VkPipeline &graphicsPipeline() { return m_graphicsPipeline; }
    const VkPipelineLayout pipelineLayout() { return m_pipelineLayout; }

    VkResult renderObject(VkCommandBuffer &cmdBuf,
                          const VulkanInstancesManager &instances,
                          VkDescriptorSet &descriptorSceneData,
                          VkDescriptorSet &descriptorInstanceData,
                          VkDescriptorSet &descriptorLightData,
                          VkDescriptorSet descriptorSkybox,
                          VkDescriptorSet &descriptorMaterials,
                          VkDescriptorSet &descriptorTextures,
                          VkDescriptorSet &descriptorTLAS,
                          SceneObject *object,
                          const SceneObjectVector &lights) const;

protected:
    VulkanContext &m_ctx;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutSceneData, m_descriptorSetLayoutInstanceData, m_descriptorSetLayoutLightData,
        m_descriptorSetLayoutSkybox, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures, m_descriptorSetLayoutTLAS;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    /* Pipeline creation */
    VkResult createPipeline(const VulkanRenderPassDeferred &renderPass, const std::string &fragmentShader);
};

}  // namespace vengine

#endif