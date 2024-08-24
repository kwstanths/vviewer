#include "VulkanRendererLambert.hpp"

#include <debug_tools/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"
#include "utils/Algorithms.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererLambert::VulkanRendererLambert(VulkanContext &context)
    : VulkanRendererForward(context)
    , m_ctx(context)
{
}

VkResult VulkanRendererLambert::initResources(VkDescriptorSetLayout cameraDescriptorLayout,
                                              VkDescriptorSetLayout instanceDataDescriptorLayout,
                                              VkDescriptorSetLayout lightDataDescriptorLayout,
                                              VkDescriptorSetLayout skyboxDescriptorLayout,
                                              VkDescriptorSetLayout materialDescriptorLayout,
                                              VkDescriptorSetLayout texturesDescriptorLayout,
                                              VkDescriptorSetLayout tlasDescriptorLayout)
{
    VULKAN_CHECK_CRITICAL(VulkanRendererForward::initResources(cameraDescriptorLayout,
                                                               instanceDataDescriptorLayout,
                                                               lightDataDescriptorLayout,
                                                               skyboxDescriptorLayout,
                                                               materialDescriptorLayout,
                                                               texturesDescriptorLayout,
                                                               tlasDescriptorLayout));

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::initSwapChainResources(VkExtent2D swapchainExtent, VulkanRenderPassDeferred renderPass)
{
    VULKAN_CHECK_CRITICAL(
        VulkanRendererForward::initSwapChainResources(swapchainExtent, renderPass, "shaders/SPIRV/lambertForward.frag.spv"));

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseSwapChainResources()
{
    VULKAN_CHECK_CRITICAL(VulkanRendererForward::releaseSwapChainResources());

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseResources()
{
    VULKAN_CHECK_CRITICAL(VulkanRendererForward::releaseResources());

    return VK_SUCCESS;
}

}  // namespace vengine