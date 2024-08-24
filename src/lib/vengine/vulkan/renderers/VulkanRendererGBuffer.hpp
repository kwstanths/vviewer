#ifndef __VulkanRendererGBuffer_hpp__
#define __VulkanRendererGBuffer_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanInstances.hpp"
#include "vulkan/VulkanRenderPass.hpp"

namespace vengine
{

class VulkanRendererGBuffer
{
public:
    VulkanRendererGBuffer(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout sceneDataDescriptorLayout,
                           VkDescriptorSetLayout instanceDataDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VkDescriptorSetLayout texturesDescriptorLayout);

    VkResult initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassDeferred &renderPass);

    VkResult releaseResources();
    VkResult releaseSwapChainResources();

    const VkPipeline &graphicsPipeline() { return m_graphicsPipeline; }
    const VkPipelineLayout &pipelineLayout() { return m_pipelineLayout; }

    VkResult renderOpaqueInstances(VkCommandBuffer &cmdBuf,
                                   const VulkanInstancesManager &instances,
                                   VkDescriptorSet &descriptorSceneData,
                                   VkDescriptorSet &descriptorInstanceData,
                                   VkDescriptorSet &descriptorMaterials,
                                   VkDescriptorSet &descriptorTextures) const;

private:
    VulkanContext &m_ctx;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutSceneData, m_descriptorSetLayoutInstanceData, m_descriptorSetLayoutMaterial,
        m_descriptorSetLayoutTextures;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    /* Pipeline creation */
    VkResult createPipeline(const VulkanRenderPassDeferred &renderPass);

    VkResult renderMeshGroup(VkCommandBuffer &commandBuffer,
                             const VulkanMesh *mesh,
                             const InstancesManager::MeshGroup &meshGoup) const;
};

}  // namespace vengine

#endif