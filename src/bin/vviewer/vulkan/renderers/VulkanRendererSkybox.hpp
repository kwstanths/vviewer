#ifndef __VulkanRendererSkybox_hpp__
#define __VulkanRendererSkybox_hpp__

#include <vector>

#include <core/Image.hpp>

#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/resources/VulkanMaterials.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanCubemap.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

class VulkanRendererSkybox
{
public:
    VulkanRendererSkybox();

    VkResult initResources(VkPhysicalDevice physicalDevice,
                           VkDevice device,
                           VkQueue queue,
                           VkCommandPool commandPool,
                           VkDescriptorSetLayout cameraDescriptorLayout);
    VkResult initSwapChainResources(VkExtent2D swapchainExtent, VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);

    VkResult releaseSwapChainResources();
    VkResult releaseResources();

    VkDescriptorSetLayout &descriptorSetLayout();

    /**
        Render a skybox
    */
    VkResult renderSkybox(VkCommandBuffer cmdBuf,
                          VkDescriptorSet cameraDescriptorSet,
                          int imageIndex,
                          const std::shared_ptr<VulkanMaterialSkybox> &skybox) const;

    /**
        Create a cubemap from an image, inputImage should be an HDR equirectangular projection
    */
    VkResult createCubemap(std::shared_ptr<VulkanTexture> inputImage, std::shared_ptr<VulkanCubemap> &vulkanCubemap) const;

    /**
        Create an irradiance map for a cubemap
    */
    VkResult createIrradianceMap(std::shared_ptr<VulkanCubemap> inputMap,
                                 std::shared_ptr<VulkanCubemap> &vulkanCubemap,
                                 uint32_t resolution = 32) const;

    /**
        Create a prefiltered cubemap for different rougness values for the input cubemap
    */
    VkResult createPrefilteredCubemap(std::shared_ptr<VulkanCubemap> inputMap,
                                      std::shared_ptr<VulkanCubemap> &vulkanCubemap,
                                      uint32_t resolution = 512) const;

private:
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_queue;
    VkCommandPool m_commandPool;
    VkExtent2D m_swapchainExtent;

    /* Skybox rendering pipeline */
    VkDescriptorSetLayout m_descriptorSetLayoutCamera;
    VkDescriptorSetLayout m_descriptorSetLayoutSkybox;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    VkRenderPass m_renderPass;
    VkSampleCountFlagBits m_msaaSamples;

    VkResult createGraphicsPipeline();
    VkResult createDescriptorSetsLayout();

    VulkanCube *m_cube;
};

}  // namespace vengine

#endif