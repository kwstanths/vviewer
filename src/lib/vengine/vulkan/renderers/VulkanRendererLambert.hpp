#ifndef __VulkanRendererLambert_hpp__
#define __VulkanRendererLambert_hpp__

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/renderers/VulkanRendererGBuffer.hpp"
#include "vulkan/renderers/VulkanRendererForward.hpp"

namespace vengine
{

class VulkanRendererLambert : VulkanRendererForward
{
    friend class VulkanRenderer;

public:
    VulkanRendererLambert(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout cameraDescriptorLayout,
                           VkDescriptorSetLayout instanceDataDescriptorLayout,
                           VkDescriptorSetLayout lightDataDescriptorLayout,
                           VkDescriptorSetLayout skyboxDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VkDescriptorSetLayout texturesDescriptorLayout,
                           VkDescriptorSetLayout tlasDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, VulkanRenderPassDeferred renderPass);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

private:
    VulkanContext &m_ctx;
};

}  // namespace vengine

#endif