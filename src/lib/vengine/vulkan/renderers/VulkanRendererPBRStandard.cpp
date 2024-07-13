#include "VulkanRendererPBRStandard.hpp"

#include <debug_tools/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"
#include "utils/Algorithms.hpp"

#include "vulkan/VulkanSceneObject.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererPBR::VulkanRendererPBR()
{
}

VkResult VulkanRendererPBR::initResources(VkPhysicalDevice physicalDevice,
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
                                                            textures.descriptorSetLayout(),
                                                            tlasDescriptorLayout));

    VULKAN_CHECK_CRITICAL(createBRDFLUT(textures));

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::initSwapChainResources(VkExtent2D swapchainExtent,
                                                   VkRenderPass renderPass,
                                                   uint32_t swapchainImages,
                                                   VkSampleCountFlagBits msaaSamples)
{
    VULKAN_CHECK_CRITICAL(VulkanRendererBase::initSwapChainResources(swapchainExtent, renderPass, swapchainImages, msaaSamples));

    {
        VkShaderModule vertexShader = VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv");
        VkShaderModule fragmentShader = VulkanShader::load(m_device, "shaders/SPIRV/pbrForwardOpaque.frag.spv");

        VULKAN_CHECK_CRITICAL(
            createPipelineForwardOpaque(vertexShader, fragmentShader, m_pipelineLayoutForwardOpaque, m_graphicsPipelineForwardOpaque));

        vkDestroyShaderModule(m_device, vertexShader, nullptr);
        vkDestroyShaderModule(m_device, fragmentShader, nullptr);
    }

    {
        VkShaderModule vertexShader = VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv");
        VkShaderModule fragmentShader = VulkanShader::load(m_device, "shaders/SPIRV/pbrForwardTransparent.frag.spv");

        VULKAN_CHECK_CRITICAL(createPipelineForwardTransparent(
            vertexShader, fragmentShader, m_pipelineLayoutForwardTransparent, m_graphicsPipelineForwardTransparent));

        vkDestroyShaderModule(m_device, vertexShader, nullptr);
        vkDestroyShaderModule(m_device, fragmentShader, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineForwardOpaque, nullptr);
    vkDestroyPipeline(m_device, m_graphicsPipelineForwardTransparent, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutForwardOpaque, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutForwardTransparent, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::releaseResources()
{
    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::createBRDFLUT(VulkanTextures &textures, uint32_t resolution) const
{
    uint32_t imageWidth = resolution;
    uint32_t imageHeight = resolution;
    VkFormat format = VK_FORMAT_R16G16_SFLOAT;

    /* Initialize image data for the BRDF LUT texture */
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageCreateInfo imageInfo = vkinit::imageCreateInfo({imageWidth, imageHeight, 1},
                                                          format,
                                                          1,
                                                          VK_SAMPLE_COUNT_1_BIT,
                                                          VK_IMAGE_TILING_OPTIMAL,
                                                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    createImage(m_physicalDevice, m_device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, imageMemory);

    VkImageView imageView;
    VkImageViewCreateInfo imageViewInfo = vkinit::imageViewCreateInfo(image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &imageViewInfo, nullptr, &imageView));

    VkSampler imageSampler;
    VkSamplerCreateInfo samplerCreateInfo =
        vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
    VULKAN_CHECK_CRITICAL(vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &imageSampler));

    /* Create render pass to render the BRDF LUT texture */
    VkRenderPass renderPass;
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;              /* Before the subpass */
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* After the subpass */
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; /* During the subpass */

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef; /* Same as shader locations */

        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstSubpass = 0;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = vkinit::renderPassCreateInfo(1, &colorAttachment, 1, &subpass, 2, dependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &renderPass));
    }

    /* Create the framebuffer */
    VkFramebuffer framebuffer;
    {
        VkFramebufferCreateInfo framebufferInfo = vkinit::framebufferCreateInfo(renderPass, 1, &imageView, imageWidth, imageHeight);
        VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &framebuffer));
    }

    /* Descriptor set layout for the render, no input */
    VkDescriptorSetLayout descriptorSetlayout;
    {
        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutInfo = vkinit::descriptorSetLayoutCreateInfo(0, nullptr);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorsetlayoutInfo, nullptr, &descriptorSetlayout));
    }

    /* Prepare graphics pipeline */
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(1, &descriptorSetlayout, 0, nullptr);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelinelayout));

        /* Pipeline stages */
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo rasterizer =
            vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState colorBlendAttachment =
            vkinit::pipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
        VkPipelineDepthStencilStateCreateInfo depthStencil =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
        VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStatesInfo =
            vkinit::pipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data());
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/quad.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/genBRDFLUT.frag.spv"), "main");
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};
        /* full screen quad render, no input */
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr);

        VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(pipelinelayout, renderPass, 0);
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicStatesInfo;
        VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    }

    /* Render the BRDF LUT texture */
    {
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderPassInfo = vkinit::renderPassBeginInfo(renderPass, framebuffer, 1, clearValues);
        renderPassInfo.renderArea.extent.width = imageWidth;
        renderPassInfo.renderArea.extent.height = imageHeight;

        /* Create a command buffer */
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, 1);
        VkCommandBuffer commandBuffer;
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));

        /* Start recording commands */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        /* Set viewport and scissor values */
        VkViewport viewport = vkinit::viewport(imageWidth, imageHeight, 0.0F, 1.0F);
        VkRect2D scissor = vkinit::rect2D(imageWidth, imageHeight, 0, 0);

        /* Perform render */
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        /* Render quad */
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        /* transition image to shader read */
        transitionImageLayout(
            commandBuffer, image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

        VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBuffer));

        /* Submit and wait */
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);

        /* Create fence to ensure that the command buffer has finished executing */
        VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo(0);
        VkFence fence;
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));
        VULKAN_CHECK_CRITICAL(vkQueueSubmit(m_queue, 1, &submitInfo, fence));
        VULKAN_CHECK_CRITICAL(vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_100S));

        VULKAN_CHECK_CRITICAL(vkQueueWaitIdle(m_queue));

        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

    vkDestroyPipeline(m_device, pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, pipelinelayout, nullptr);
    vkDestroyRenderPass(m_device, renderPass, nullptr);
    vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    vkDestroyDescriptorSetLayout(m_device, descriptorSetlayout, nullptr);

    /* Create texture resource */
    auto vktex = new VulkanTexture(AssetInfo("PBR_BRDF_LUT", AssetSource::INTERNAL),
                                   ColorSpace::LINEAR,
                                   ColorDepth::BITS16,
                                   imageWidth,
                                   imageHeight,
                                   2U,
                                   image,
                                   imageMemory,
                                   imageView,
                                   imageSampler,
                                   format,
                                   0);

    /* Add to resources */
    auto &texturesMap = AssetManager::getInstance().texturesMap();
    texturesMap.add(vktex);

    /* Add texture to textures descriptor managemenet */
    textures.addTexture(vktex);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::renderObjectsForwardOpaque(VkCommandBuffer &cmdBuf,
                                                       const VulkanInstancesManager &instances,
                                                       VkDescriptorSet &descriptorScene,
                                                       VkDescriptorSet &descriptorModel,
                                                       VkDescriptorSet &descriptorLight,
                                                       VkDescriptorSet descriptorSkybox,
                                                       VkDescriptorSet &descriptorMaterials,
                                                       VkDescriptorSet &descriptorTextures,
                                                       VkDescriptorSet &descriptorTLAS,
                                                       const SceneGraph &lights) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineForwardOpaque);

    std::array<VkDescriptorSet, 7> descriptorSets = {
        descriptorScene, descriptorModel, descriptorLight, descriptorMaterials, descriptorTextures, descriptorSkybox, descriptorTLAS};
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutForwardOpaque,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            0,
                            nullptr);

    VulkanRendererBase::renderMaterialGroup(
        cmdBuf, instances.materialGroup(MaterialType::MATERIAL_PBR_STANDARD), m_pipelineLayoutForwardOpaque, lights);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::renderObjectsForwardTransparent(VkCommandBuffer &cmdBuf,
                                                            VulkanInstancesManager &instances,
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

    VulkanRendererBase::renderObjects(cmdBuf, instances, m_pipelineLayoutForwardTransparent, {object}, lights);

    return VK_SUCCESS;
}

}  // namespace vengine