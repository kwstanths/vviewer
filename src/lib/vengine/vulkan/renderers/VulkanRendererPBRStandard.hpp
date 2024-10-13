#ifndef __VulkanRendererPBR_hpp__
#define __VulkanRendererPBR_hpp__

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTextures.hpp"
#include "vulkan/renderers/VulkanRendererForward.hpp"

namespace vengine
{

class VulkanRendererPBR : VulkanRendererForward
{
    friend class VulkanRenderer;

public:
    VulkanRendererPBR(VulkanContext &context);

    VkResult initResources(VkDescriptorSetLayout cameraDescriptorLayout,
                           VkDescriptorSetLayout instanceDataDescriptorLayout,
                           VkDescriptorSetLayout lightDataDescriptorLayout,
                           VkDescriptorSetLayout skyboxDescriptorLayout,
                           VkDescriptorSetLayout materialDescriptorLayout,
                           VulkanTextures &textures,
                           VkDescriptorSetLayout tlasDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, VulkanRenderPassDeferred renderPass);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

private:
    VulkanContext &m_ctx;

    VkResult createBRDFLUT(VulkanTextures &textures, uint32_t resolution = 128) const;
};

}  // namespace vengine

#endif