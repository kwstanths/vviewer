#ifndef __VulkanRendererOverlay_hpp__
#define __VulkanRendererOverlay_hpp__

#include "core/Camera.hpp"
#include "math/AABB.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/VulkanFramebuffer.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterial.hpp"
#include "vulkan/resources/VulkanModel3D.hpp"
#include "vulkan/VulkanRenderPass.hpp"

namespace vengine
{

class VulkanRendererOverlay
{
    friend class VulkanRenderer;

public:
    VulkanRendererOverlay(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout cameraDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, const VulkanRenderPassOverlay &renderPass);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    /* Draw a transform widget at a certain position */
    VkResult render3DTransform(VkCommandBuffer &cmdBuf,
                               VkDescriptorSet &descriptorScene,
                               uint32_t imageIndex,
                               const glm::mat4 &modelMatrix,
                               const std::shared_ptr<Camera> &camera) const;

    /* Draw an AABB */
    VkResult renderAABB3(VkCommandBuffer &cmdBuf,
                         VkDescriptorSet &descriptorScene,
                         uint32_t imageIndex,
                         const AABB3 &aabb,
                         const std::shared_ptr<Camera> &camera) const;

private:
    VulkanContext &m_ctx;
    VkExtent2D m_swapchainExtent;

    VkDescriptorSetLayout m_descriptorSetLayoutCamera;

    VkPipelineLayout m_pipelineLayout3DTransform, m_pipelineLayoutAABB3;
    VkPipeline m_graphicsPipeline3DTransform, m_graphicsPipelineAABB3;

    VulkanModel3D *m_arrow = nullptr;
    ID m_IdX, m_IdY, m_IdZ;

    VkResult createGraphicsPipeline3DTransform(const VulkanRenderPassOverlay &renderPass);
    VkResult createGraphicsPipelineAABB3(const VulkanRenderPassOverlay &renderPass);
};

}  // namespace vengine

#endif