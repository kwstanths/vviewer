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

VulkanRendererLambert::VulkanRendererLambert()
{
}

VkResult VulkanRendererLambert::initResources(VkPhysicalDevice physicalDevice,
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
                                              VkDescriptorSetLayout tlasDescriptorLayout)
{
    VULKAN_CHECK_CRITICAL(VulkanRendererBase::initResources(physicalDevice,
                                                            device,
                                                            queue,
                                                            commandPool,
                                                            physicalDeviceProperties,
                                                            cameraDescriptorLayout,
                                                            modelDescriptorLayout,
                                                            lightDescriptorLayout,
                                                            skyboxDescriptorLayout,
                                                            materialDescriptorLayout,
                                                            texturesDescriptorLayout,
                                                            tlasDescriptorLayout));

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::initSwapChainResources(VkExtent2D swapchainExtent,
                                                       VkRenderPass renderPass,
                                                       uint32_t swapchainImages,
                                                       VkSampleCountFlagBits msaaSamples)
{
    VULKAN_CHECK_CRITICAL(VulkanRendererBase::initSwapChainResources(swapchainExtent, renderPass, swapchainImages, msaaSamples));

    VkShaderModule vertexShader = VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv");
    VkShaderModule fragmentShader = VulkanShader::load(m_device, "shaders/SPIRV/lambertBase.frag.spv");

    VULKAN_CHECK_CRITICAL(
        createPipelineForwardOpaque(vertexShader, fragmentShader, m_pipelineLayoutForwardOpaque, m_graphicsPipelineForwardOpaque));
    VULKAN_CHECK_CRITICAL(createPipelineForwardTransparent(
        vertexShader, fragmentShader, m_pipelineLayoutForwardTransparent, m_graphicsPipelineForwardTransparent));

    vkDestroyShaderModule(m_device, vertexShader, nullptr);
    vkDestroyShaderModule(m_device, fragmentShader, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineForwardOpaque, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipelineForwardTransparent, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutForwardOpaque, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutForwardTransparent, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::renderObjectsForwardOpaque(VkCommandBuffer &cmdBuf,
                                                           VkDescriptorSet &descriptorScene,
                                                           VkDescriptorSet &descriptorModel,
                                                           VkDescriptorSet &descriptorLight,
                                                           VkDescriptorSet descriptorSkybox,
                                                           VkDescriptorSet &descriptorMaterial,
                                                           VkDescriptorSet &descriptorTextures,
                                                           VkDescriptorSet &descriptorTLAS,
                                                           const SceneGraph &objects,
                                                           const SceneGraph &lights) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineForwardOpaque);

    std::array<VkDescriptorSet, 7> descriptorSets = {
        descriptorScene,
        descriptorModel,
        descriptorLight,
        descriptorMaterial,
        descriptorTextures,
        descriptorSkybox,
        descriptorTLAS,
    };
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutForwardOpaque,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    VulkanRendererBase::renderObjects(cmdBuf, m_pipelineLayoutForwardOpaque, objects, lights);

    return VK_SUCCESS;
}

VkResult VulkanRendererLambert::renderObjectsForwardTransparent(VkCommandBuffer &cmdBuf,
                                                                VkDescriptorSet &descriptorScene,
                                                                VkDescriptorSet &descriptorModel,
                                                                VkDescriptorSet &descriptorLight,
                                                                VkDescriptorSet descriptorSkybox,
                                                                VkDescriptorSet &descriptorMaterials,
                                                                VkDescriptorSet &descriptorTextures,
                                                                VkDescriptorSet &descriptorTLAS,
                                                                SceneObject *object,
                                                                const SceneGraph &lights) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineForwardTransparent);

    std::array<VkDescriptorSet, 7> descriptorSets = {
        descriptorScene, descriptorModel, descriptorLight, descriptorMaterials, descriptorTextures, descriptorSkybox, descriptorTLAS};
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutForwardTransparent,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    VulkanRendererBase::renderObjects(cmdBuf, m_pipelineLayoutForwardTransparent, {object}, lights);

    return VK_SUCCESS;
}

}  // namespace vengine