#include "VulkanRendererSkybox.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <debug_tools/Console.hpp>

#include <math/Constants.hpp>
#include <core/AssetManager.hpp>
#include <vulkan/vulkan_core.h>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanRendererSkybox::VulkanRendererSkybox()
{
}

VkResult VulkanRendererSkybox::initResources(VkPhysicalDevice physicalDevice,
                                             VkDevice device,
                                             VkQueue queue,
                                             VkCommandPool commandPool,
                                             VkDescriptorSetLayout cameraDescriptorLayout)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_queue = queue;
    m_commandPool = commandPool;
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayout());

    m_cube = new VulkanCube(m_physicalDevice, m_device, queue, commandPool);

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::initSwapChainResources(VkExtent2D swapchainExtent,
                                                      VkRenderPass renderPass,
                                                      VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    VULKAN_CHECK_CRITICAL(createGraphicsPipeline());

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::releaseResources()
{
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutSkybox, nullptr);

    /* Destroy equi transform resources */

    m_cube->destroy(m_device);

    return VK_SUCCESS;
}

VkDescriptorSetLayout &VulkanRendererSkybox::descriptorSetLayout()
{
    return m_descriptorSetLayoutSkybox;
}

VkResult VulkanRendererSkybox::renderSkybox(VkCommandBuffer cmdBuf,
                                            VkDescriptorSet cameraDescriptorSet,
                                            int imageIndex,
                                            const std::shared_ptr<VulkanMaterialSkybox> &skybox) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

    {
        VkBuffer vertexBuffers[] = {m_cube->vertexBuffer().buffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, m_cube->indexBuffer().buffer(), 0, m_cube->indexType());

        VkDescriptorSet descriptorSets[2] = {cameraDescriptorSet, skybox->getDescriptor(imageIndex)};

        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 2, &descriptorSets[0], 0, nullptr);

        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(m_cube->indices().size()), 1, 0, 0, 0);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::createCubemap(std::shared_ptr<VulkanTexture> inputImage,
                                             std::shared_ptr<VulkanCubemap> &vulkanCubemap) const
{
    uint32_t cubemapWidth = static_cast<uint32_t>(std::min(inputImage->width() / 4, uint32_t(1080)));
    uint32_t cubemapHeight = cubemapWidth;
    uint32_t numMips = static_cast<uint32_t>(std::floor(std::log2(std::max(cubemapWidth, cubemapHeight)))) + 1;
    VkFormat format = inputImage->format();

    VkImage cubemapImage;
    VkDeviceMemory cubemapMemory;
    VkImageView cubemapImageView;
    VkSampler cubemapSampler;
    /* Initialize cubemap image data */
    {
        VkImageCreateInfo cubemapImageInfo =
            vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                    format,
                                    numMips,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        cubemapImageInfo.arrayLayers = 6;                             /* Cube faces count as array layers */
        cubemapImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; /* This flag is required for cube map images */
        createImage(m_physicalDevice, m_device, cubemapImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubemapImage, cubemapMemory);

        /* Create image view */
        VkImageViewCreateInfo viewInfo = vkinit::imageViewCreateInfo(cubemapImage, format, VK_IMAGE_ASPECT_COLOR_BIT, numMips);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.subresourceRange.layerCount = 6;
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &viewInfo, nullptr, &cubemapImageView));

        VkSamplerCreateInfo cubemapSamplerInfo =
            vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
        cubemapSamplerInfo.maxLod = static_cast<float>(numMips);
        VULKAN_CHECK_CRITICAL(vkCreateSampler(m_device, &cubemapSamplerInfo, nullptr, &cubemapSampler));

        /* Transition cubemap faces to be optimal to copy data to them */
        transitionImageLayout(m_device,
                              m_queue,
                              m_commandPool,
                              cubemapImage,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              numMips,
                              6);
    }
    /* Create render pass to render each face from the equirectangular input texture to an offscreen render target */
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

        // Renderpass
        VkRenderPassCreateInfo renderPassCreateInfo =
            vkinit::renderPassCreateInfo(1, &colorAttachment, 1, &subpass, 2, dependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &renderPass));
    }

    /* Prepare the render target and the framebuffer */
    struct {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkFramebuffer framebuffer;
    } offscreen;

    {
        VkImageCreateInfo offscreenTargetImageInfo =
            vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                    format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(m_physicalDevice,
                    m_device,
                    offscreenTargetImageInfo,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    offscreen.image,
                    offscreen.memory);

        VkImageViewCreateInfo colorImageView = vkinit::imageViewCreateInfo(offscreen.image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &colorImageView, nullptr, &offscreen.view));

        /* Create the framebuffer */
        VkFramebufferCreateInfo framebufferInfo =
            vkinit::framebufferCreateInfo(renderPass, 1, &offscreen.view, cubemapWidth, cubemapHeight);
        VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &offscreen.framebuffer));

        /* Transition offscreen render target to color attachment optimal */
        transitionImageLayout(
            m_device, m_queue, m_commandPool, offscreen.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
    }

    /* Push constant to hold the mvp matrix for each cubemap face render */
    struct PushBlock {
        glm::mat4 mvp;
    } pushBlock;

    /* Descriptor set layout for the renders, the only input would be the input texture */
    VkDescriptorSetLayout descriptorSetlayout;
    {
        VkDescriptorSetLayoutBinding binding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(1, &binding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorsetlayoutCreateInfo, nullptr, &descriptorSetlayout));
    }

    /* Prepare the graphics pipeline */
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;
    {
        VkPushConstantRange pushConstantRange =
            vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
            vkinit::pipelineLayoutCreateInfo(1, &descriptorSetlayout, 1, &pushConstantRange);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &pipelinelayout));

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxFilterCube.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxCubemapWrite.frag.spv"), "main");
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VulkanVertex::getBindingDescription();
        auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
            1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo rasterizer =
            vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
        VkPipelineDepthStencilStateCreateInfo depthStencil =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
        VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStates =
            vkinit::pipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStateEnables.size()), dynamicStateEnables.data());

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
        pipelineInfo.pDynamicState = &dynamicStates;
        VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    }

    /* Create descriptor pool, and descriptor set for the renders */
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    {
        VkDescriptorPoolSize poolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vkinit::descriptorPoolCreateInfo(1, &poolSize, 1);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(descriptorPool, 1, &descriptorSetlayout);
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet));

        /* Write the descriptor set, bind with the input image. We should use a separate sampler here i guess, but that should be ok
         * anyway
         */
        VkDescriptorImageInfo imageInfo =
            vkinit::descriptorImageInfo(inputImage->sampler(), inputImage->imageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet writeDescriptorSet =
            vkinit::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfo);
        vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, nullptr);
    }

    /* Render cubemap faces */
    {
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, offscreen.framebuffer, 1, clearValues);
        renderPassBeginInfo.renderArea.extent.width = cubemapWidth;
        renderPassBeginInfo.renderArea.extent.height = cubemapHeight;

        /* mvp matrices for each cubemap face */
        std::vector<glm::mat4> matrices = {
            // POSITIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        /* Create command buffer */
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, 1);
        VkCommandBuffer commandBuffer;
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
        /* Start recording commands */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkViewport viewport = vkinit::viewport(cubemapWidth, cubemapHeight, 0.0F, 1.0F);
        VkRect2D scissor = vkinit::rect2D(cubemapWidth, cubemapHeight, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        /* Render each face to a different layer in the cubemap image */
        /* Render each cubemap face, and copy the results from the offscreen
         * render target to the cubemap */
        for (uint32_t f = 0; f < 6; f++) {
            vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Update shader push constant block
            pushBlock.mvp = glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

            vkCmdPushConstants(commandBuffer,
                               pipelinelayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PushBlock),
                               &pushBlock);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorSet, 0, NULL);

            /* Render cube */
            VkBuffer vertexBuffers[] = {m_cube->vertexBuffer().buffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, m_cube->indexBuffer().buffer(), 0, m_cube->indexType());
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_cube->indices().size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            /* Transition render target for data transfer */
            transitionImageLayout(
                commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

            // Create copy region for transfer from framebuffer render target to
            // corresponding cube face
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = {0, 0, 0};

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = f;
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = {0, 0, 0};

            copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
            copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
            copyRegion.extent.depth = 1;

            vkCmdCopyImage(commandBuffer,
                           offscreen.image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           cubemapImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);

            /* Transition render target to color attachment again */
            transitionImageLayout(
                commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
        }

        /* Transition cubemap to transfer src in order to create mip map levels */
        transitionImageLayout(
            commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1, 6);

        /* Generate mip map levels for each cubemap face */
        for (int f = 0; f < 6; f++) {
            for (uint32_t m = 1; m < numMips; m++) {
                VkImageBlit imageBlit{};

                // Source is the original image
                imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.srcSubresource.baseArrayLayer = f;
                imageBlit.srcSubresource.layerCount = 1;
                imageBlit.srcSubresource.mipLevel = 0;
                imageBlit.srcOffsets[1].x = static_cast<int32_t>(cubemapWidth);
                imageBlit.srcOffsets[1].y = static_cast<int32_t>(cubemapHeight);
                imageBlit.srcOffsets[1].z = 1;

                // Destination is the corresponding mip map level
                imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageBlit.dstSubresource.baseArrayLayer = f;
                imageBlit.dstSubresource.layerCount = 1;
                imageBlit.dstSubresource.mipLevel = m;
                imageBlit.dstOffsets[1].x = static_cast<int32_t>(cubemapWidth * std::pow(0.5f, m));
                imageBlit.dstOffsets[1].y = static_cast<int32_t>(cubemapHeight * std::pow(0.5f, m));
                imageBlit.dstOffsets[1].z = 1;

                VkImageSubresourceRange subresourceRange = {};
                subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                subresourceRange.baseMipLevel = m;
                subresourceRange.levelCount = 1;
                subresourceRange.baseArrayLayer = f;
                subresourceRange.layerCount = 1;

                // change layout of current mip level to transfer destination
                transitionImageLayout(
                    commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

                // Do blit operation from original level
                vkCmdBlitImage(commandBuffer,
                               cubemapImage,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               cubemapImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &imageBlit,
                               VK_FILTER_LINEAR);
            }
        }

        /* Transition original 0 mip level image from src optimal to shader read only */
        transitionImageLayout(
            commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

        /* Transition rest of mip levels images from dstoptimal to shader read only */
        VkImageSubresourceRange mipLevelsTransition{};
        mipLevelsTransition.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipLevelsTransition.baseArrayLayer = 0;
        mipLevelsTransition.baseMipLevel = 1;
        mipLevelsTransition.layerCount = 6;
        mipLevelsTransition.levelCount = numMips - 1;
        transitionImageLayout(commandBuffer,
                              cubemapImage,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              mipLevelsTransition);

        VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBuffer));

        /* Submit and wait for end */
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo();
        VkFence fence;
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));

        VULKAN_CHECK_CRITICAL(vkQueueSubmit(m_queue, 1, &submitInfo, fence));
        VULKAN_CHECK_CRITICAL(vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_10S));

        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

    vkDestroyRenderPass(m_device, renderPass, nullptr);
    vkDestroyFramebuffer(m_device, offscreen.framebuffer, nullptr);
    vkFreeMemory(m_device, offscreen.memory, nullptr);
    vkDestroyImageView(m_device, offscreen.view, nullptr);
    vkDestroyImage(m_device, offscreen.image, nullptr);
    vkDestroyDescriptorPool(m_device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_device, descriptorSetlayout, nullptr);
    vkDestroyPipeline(m_device, pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, pipelinelayout, nullptr);

    /* Added created cubemaps to the cubemap assets */
    auto &cubemaps = AssetManager::getInstance().cubemapsMap();
    vulkanCubemap = std::make_shared<VulkanCubemap>(inputImage->name(), cubemapImage, cubemapMemory, cubemapImageView, cubemapSampler);
    cubemaps.add(vulkanCubemap);

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::createIrradianceMap(std::shared_ptr<VulkanCubemap> inputMap,
                                                   std::shared_ptr<VulkanCubemap> &vulkanCubemap,
                                                   uint32_t resolution) const
{
    /* Cubemap data */
    VkImage cubemapImage;
    VkDeviceMemory cubemapMemory;
    VkImageView cubemapImageView;
    VkSampler cubemapSampler;

    uint32_t cubemapWidth = 64;
    uint32_t cubemapHeight = 64;
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;

    /* Initialize cubemap image */
    {
        VkImageCreateInfo imageInfo = vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                                              format,
                                                              1,
                                                              VK_SAMPLE_COUNT_1_BIT,
                                                              VK_IMAGE_TILING_OPTIMAL,
                                                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        imageInfo.arrayLayers = 6;                             /* Cube faces count as array layers  */
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; /* This flag is required for cube map images */
        createImage(m_physicalDevice, m_device, imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubemapImage, cubemapMemory);

        VkImageViewCreateInfo viewInfo = vkinit::imageViewCreateInfo(cubemapImage, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.subresourceRange.layerCount = 6;
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &viewInfo, nullptr, &cubemapImageView));

        VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
        VULKAN_CHECK_CRITICAL(vkCreateSampler(m_device, &samplerInfo, nullptr, &cubemapSampler));

        /* Transition cubemap faces to be optimal to copy data to them */
        transitionImageLayout(
            m_device, m_queue, m_commandPool, cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 6);
    }

    /* Create render pass to render each face of the irradiance cubemap to an offscreen render target */
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

        // Renderpass
        VkRenderPassCreateInfo renderPassCreateInfo =
            vkinit::renderPassCreateInfo(1, &colorAttachment, 1, &subpass, 2, dependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &renderPass));
    }

    /* Prepare the render target and the framebuffer */
    struct {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkFramebuffer framebuffer;
    } offscreen;
    {
        VkImageCreateInfo imageCreateInfo =
            vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                    format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(
            m_physicalDevice, m_device, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreen.image, offscreen.memory);

        VkImageViewCreateInfo colorImageView = vkinit::imageViewCreateInfo(offscreen.image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &colorImageView, nullptr, &offscreen.view));

        VkFramebufferCreateInfo framebufferInfo =
            vkinit::framebufferCreateInfo(renderPass, 1, &offscreen.view, cubemapWidth, cubemapHeight);
        VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &offscreen.framebuffer));

        /* Transition offscreen render target to color attachment optimal */
        transitionImageLayout(
            m_device, m_queue, m_commandPool, offscreen.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
    }

    /* Push constant to hold the mvp matrix for each cubemap face render, and the delta phi and delta theta for the convolution */
    struct PushBlock {
        glm::mat4 mvp = glm::mat4(1.0f);
        float deltaPhi = (2.0f * float(PI)) / 180.0f;
        float deltaTheta = (0.5f * float(PI)) / 64.0f;
    } pushBlock;

    /* Descriptor set layout for the renders, the only input would be the input texture */
    VkDescriptorSetLayout descriptorSetlayout;
    {
        VkDescriptorSetLayoutBinding binding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(1, &binding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorsetlayoutCreateInfo, nullptr, &descriptorSetlayout));
    }

    /* Prepare the graphics pipeline */
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;
    {
        VkPushConstantRange pushConstantRange =
            vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlock), 0);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
            vkinit::pipelineLayoutCreateInfo(1, &descriptorSetlayout, 1, &pushConstantRange);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &pipelinelayout));

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxFilterCube.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxCubemapIrradiance.frag.spv"), "main");
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VulkanVertex::getBindingDescription();
        auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
            1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo rasterizer =
            vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
        VkPipelineDepthStencilStateCreateInfo depthStencil =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
        VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStates =
            vkinit::pipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStateEnables.size()), dynamicStateEnables.data());

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
        pipelineInfo.pDynamicState = &dynamicStates;
        VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    }

    /* Create a descriptor pool and a descriptor set for the render */
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    {
        VkDescriptorPoolSize poolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vkinit::descriptorPoolCreateInfo(1, &poolSize, 1);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(descriptorPool, 1, &descriptorSetlayout);
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet));

        /* Write the descriptor set, bind with the input image. We should use a separate sampler here i guess, but that should be
         * ok anyway */
        VkDescriptorImageInfo imageInfo =
            vkinit::descriptorImageInfo(inputMap->getSampler(), inputMap->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet writeDescriptorSet =
            vkinit::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfo);
        vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, nullptr);
    }

    /* Render cubemap faces */
    {
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, offscreen.framebuffer, 1, clearValues);
        renderPassBeginInfo.renderArea.extent.width = cubemapWidth;
        renderPassBeginInfo.renderArea.extent.height = cubemapHeight;

        /* mvp matrices for each cubemap face */
        std::vector<glm::mat4> matrices = {
            // POSITIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        /* Create command buffer */
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, 1);
        VkCommandBuffer commandBuffer;
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
        /* Start recording commands */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkViewport viewport = vkinit::viewport(cubemapWidth, cubemapHeight, 0.0F, 1.0F);
        VkRect2D scissor = vkinit::rect2D(cubemapWidth, cubemapHeight, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        /* Cube mesh to render */
        /* Render each cubemap face, and copy the results from the offscreen render target to the cubemap */
        for (uint32_t f = 0; f < 6; f++) {
            vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Update shader push constant block
            pushBlock.mvp = glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

            vkCmdPushConstants(commandBuffer,
                               pipelinelayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PushBlock),
                               &pushBlock);

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorSet, 0, NULL);

            /* Render cube */
            VkBuffer vertexBuffers[] = {m_cube->vertexBuffer().buffer()};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, m_cube->indexBuffer().buffer(), 0, m_cube->indexType());
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_cube->indices().size()), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            /* Transition render target for data transfer */
            transitionImageLayout(
                commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

            // Create copy region for transfer from framebuffer render target to corresponding cube face
            VkImageCopy copyRegion = {};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = {0, 0, 0};

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = f;
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = {0, 0, 0};

            copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
            copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
            copyRegion.extent.depth = 1;

            vkCmdCopyImage(commandBuffer,
                           offscreen.image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           cubemapImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &copyRegion);

            /* Transition render target to color attachment again */
            transitionImageLayout(
                commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
        }

        /* Transition cubemap to shader read only */
        transitionImageLayout(
            commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);

        vkEndCommandBuffer(commandBuffer);

        /* Submit and wait for end */
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo();
        VkFence fence;
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));
        VULKAN_CHECK_CRITICAL(vkQueueSubmit(m_queue, 1, &submitInfo, fence));
        VULKAN_CHECK_CRITICAL(vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_10S));

        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

    vkDestroyRenderPass(m_device, renderPass, nullptr);
    vkDestroyFramebuffer(m_device, offscreen.framebuffer, nullptr);
    vkFreeMemory(m_device, offscreen.memory, nullptr);
    vkDestroyImageView(m_device, offscreen.view, nullptr);
    vkDestroyImage(m_device, offscreen.image, nullptr);
    vkDestroyDescriptorPool(m_device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_device, descriptorSetlayout, nullptr);
    vkDestroyPipeline(m_device, pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, pipelinelayout, nullptr);

    auto &cubemaps = AssetManager::getInstance().cubemapsMap();
    vulkanCubemap = std::make_shared<VulkanCubemap>(
        inputMap->name() + "_irradiance", cubemapImage, cubemapMemory, cubemapImageView, cubemapSampler);
    cubemaps.add(vulkanCubemap);

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::createPrefilteredCubemap(std::shared_ptr<VulkanCubemap> inputMap,
                                                        std::shared_ptr<VulkanCubemap> &vulkanCubemap,
                                                        uint32_t resolution) const
{
    const uint32_t cubemapWidth = 512;
    const uint32_t cubemapHeight = 512;
    const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
    const uint32_t numMips = static_cast<uint32_t>(floor(log2(resolution))) + 1;

    /* Cubemap data */
    VkImage cubemapImage;
    VkDeviceMemory cubemapMemory;
    VkImageView cubemapImageView;
    VkSampler cubemapSampler;
    {
        VkImageCreateInfo imageCreateInfo = vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                                                    format,
                                                                    numMips,
                                                                    VK_SAMPLE_COUNT_1_BIT,
                                                                    VK_IMAGE_TILING_OPTIMAL,
                                                                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        imageCreateInfo.arrayLayers = 6;                             /* Cube faces count as array layers */
        imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; /* This flag is required for cube map images */
        createImage(m_physicalDevice, m_device, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, cubemapImage, cubemapMemory);

        VkImageViewCreateInfo viewInfo = vkinit::imageViewCreateInfo(cubemapImage, format, VK_IMAGE_ASPECT_COLOR_BIT, numMips);
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.subresourceRange.layerCount = 6;
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &viewInfo, nullptr, &cubemapImageView));

        VkSamplerCreateInfo samplerInfo = vkinit::samplerCreateInfo(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR);
        samplerInfo.maxLod = static_cast<float>(numMips);
        VULKAN_CHECK_CRITICAL(vkCreateSampler(m_device, &samplerInfo, nullptr, &cubemapSampler));
    }

    /* Create render pass to render each face of the irradiance cubemap to an offscreen render target */
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

        // Renderpass
        VkRenderPassCreateInfo renderPassCreateInfo =
            vkinit::renderPassCreateInfo(1, &colorAttachment, 1, &subpass, 2, dependencies.data());
        VULKAN_CHECK_CRITICAL(vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &renderPass));
    }

    /* Prepare the render target and the framebuffer */
    struct {
        VkImage image;
        VkImageView view;
        VkDeviceMemory memory;
        VkFramebuffer framebuffer;
    } offscreen;
    {
        /* The color attachment */
        VkImageCreateInfo imageCreateInfo =
            vkinit::imageCreateInfo({cubemapWidth, cubemapHeight, 1},
                                    format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        createImage(
            m_physicalDevice, m_device, imageCreateInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, offscreen.image, offscreen.memory);

        VkImageViewCreateInfo colorImageView = vkinit::imageViewCreateInfo(offscreen.image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &colorImageView, nullptr, &offscreen.view));

        VkFramebufferCreateInfo framebufferInfo =
            vkinit::framebufferCreateInfo(renderPass, 1, &offscreen.view, cubemapWidth, cubemapHeight);
        VULKAN_CHECK_CRITICAL(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &offscreen.framebuffer));

        /* Transition offscreen render target to color attachment optimal */
        transitionImageLayout(
            m_device, m_queue, m_commandPool, offscreen.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
    }

    /* Push constant to hold the mvp matrix for each cubemap face render, the roughness value and the number of samples */
    struct PushBlock {
        glm::mat4 mvp = glm::mat4(1.0f);
        float roughness = 0.0f;
        uint32_t numSamples = 32u;
    } pushBlock;

    /* Descriptor set layout for the renders, the only input would be the input texture */
    VkDescriptorSetLayout descriptorSetlayout;
    {
        VkDescriptorSetLayoutBinding binding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCreateInfo = vkinit::descriptorSetLayoutCreateInfo(1, &binding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorsetlayoutCreateInfo, nullptr, &descriptorSetlayout));
    }

    /* Prepare graphics pipeline */
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;
    {
        VkPushConstantRange pushConstantRange =
            vkinit::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(pushBlock), 0);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
            vkinit::pipelineLayoutCreateInfo(1, &descriptorSetlayout, 1, &pushConstantRange);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &pipelinelayout));

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxFilterCube.vert.spv"), "main");
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skyboxCubemapPrefilteredMap.frag.spv"), "main");
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

        auto bindingDescription = VulkanVertex::getBindingDescription();
        auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
            1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo rasterizer =
            vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlending = vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
        VkPipelineDepthStencilStateCreateInfo depthStencil =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_FALSE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);
        VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicStates =
            vkinit::pipelineDynamicStateCreateInfo(static_cast<uint32_t>(dynamicStateEnables.size()), dynamicStateEnables.data());

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
        pipelineInfo.pDynamicState = &dynamicStates;
        VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

        vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);
        vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    }

    /* Create descriptor pool and descriptor for the render */
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    {
        VkDescriptorPoolSize poolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1);

        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = vkinit::descriptorPoolCreateInfo(1, &poolSize, 1);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(descriptorPool, 1, &descriptorSetlayout);
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet));

        /* Write the descriptor set, bind with the input image. We should use a separate sampler here i guess, but that should be
         * ok anyway */
        VkDescriptorImageInfo imageInfo =
            vkinit::descriptorImageInfo(inputMap->getSampler(), inputMap->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkWriteDescriptorSet writeDescriptorSet =
            vkinit::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &imageInfo);
        vkUpdateDescriptorSets(m_device, 1, &writeDescriptorSet, 0, nullptr);
    }

    /* Render cubemap faces */
    {
        VkClearValue clearValues[1];
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(renderPass, offscreen.framebuffer, 1, clearValues);
        renderPassBeginInfo.renderArea.extent.width = cubemapWidth;
        renderPassBeginInfo.renderArea.extent.height = cubemapHeight;

        /* mvp matrices for each cubemap face */
        std::vector<glm::mat4> matrices = {
            // POSITIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_X
            glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                        glm::radians(180.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Y
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // POSITIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            // NEGATIVE_Z
            glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };

        /* Create command buffer */
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, 1);
        VkCommandBuffer commandBuffer;
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
        /* Start recording commands */
        VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        VkViewport viewport = vkinit::viewport(cubemapWidth, cubemapHeight, 0.0F, 1.0F);
        VkRect2D scissor = vkinit::rect2D(cubemapWidth, cubemapHeight, 0, 0);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        /* Transition cubemap faces to be optimal to copy data to them */
        transitionImageLayout(
            commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMips, 6);

        /* Render */
        for (uint32_t m = 0; m < numMips; m++) {
            pushBlock.roughness = (float)m / (float)(numMips - 1);
            /* For each rouhgness level, render each cubemap face, and copy the results from the offscreen render target to the
             * corresponding cubemap mip level */
            for (uint32_t f = 0; f < 6; f++) {
                /* Set viewport */
                viewport.width = static_cast<float>(cubemapWidth * std::pow(0.5f, m));
                viewport.height = static_cast<float>(cubemapHeight * std::pow(0.5f, m));
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                // Update shader push constant block
                pushBlock.mvp = glm::perspective((float)(PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                vkCmdPushConstants(commandBuffer,
                                   pipelinelayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   sizeof(PushBlock),
                                   &pushBlock);

                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &descriptorSet, 0, NULL);

                /* Render cube */
                VkBuffer vertexBuffers[] = {m_cube->vertexBuffer().buffer()};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, m_cube->indexBuffer().buffer(), 0, m_cube->indexType());
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_cube->indices().size()), 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffer);

                /* Transition render target for data transfer */
                transitionImageLayout(
                    commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 1);

                // Create copy region for transfer from framebuffer render target to corresponding cube face
                VkImageCopy copyRegion = {};
                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.mipLevel = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = {0, 0, 0};

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.baseArrayLayer = f;
                copyRegion.dstSubresource.mipLevel = m;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = {0, 0, 0};

                copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
                copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                copyRegion.extent.depth = 1;

                vkCmdCopyImage(commandBuffer,
                               offscreen.image,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               cubemapImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copyRegion);

                /* Transition render target to color attachment again */
                transitionImageLayout(
                    commandBuffer, offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
            }
        }

        /* Transition cubemap to shader read only */
        transitionImageLayout(
            commandBuffer, cubemapImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, numMips, 6);

        VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBuffer));

        /* Submit and wait for end */
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);

        // Create fence to ensure that the command buffer has finished executing
        VkFenceCreateInfo fenceInfo = vkinit::fenceCreateInfo();
        VkFence fence;
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_device, &fenceInfo, nullptr, &fence));
        VULKAN_CHECK_CRITICAL(vkQueueSubmit(m_queue, 1, &submitInfo, fence));
        VULKAN_CHECK_CRITICAL(vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_10S));
        vkDestroyFence(m_device, fence, nullptr);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
    }

    vkDestroyRenderPass(m_device, renderPass, nullptr);
    vkDestroyFramebuffer(m_device, offscreen.framebuffer, nullptr);
    vkFreeMemory(m_device, offscreen.memory, nullptr);
    vkDestroyImageView(m_device, offscreen.view, nullptr);
    vkDestroyImage(m_device, offscreen.image, nullptr);
    vkDestroyDescriptorPool(m_device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_device, descriptorSetlayout, nullptr);
    vkDestroyPipeline(m_device, pipeline, nullptr);
    vkDestroyPipelineLayout(m_device, pipelinelayout, nullptr);

    auto &cubemaps = AssetManager::getInstance().cubemapsMap();
    vulkanCubemap = std::make_shared<VulkanCubemap>(
        inputMap->name() + "_prefiltered", cubemapImage, cubemapMemory, cubemapImageView, cubemapSampler);
    cubemaps.add(vulkanCubemap);

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::createDescriptorSetsLayout()
{
    VkDescriptorSetLayoutBinding skyboxTextureLayoutBinding = vkinit::descriptorSetLayoutBinding(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_MISS_BIT_KHR, 0, 1);

    VkDescriptorSetLayoutBinding irradianceTextureLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, 1);

    VkDescriptorSetLayoutBinding prefilteredTextureLayoutBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2, 1);

    std::array<VkDescriptorSetLayoutBinding, 3> setBindings = {
        skyboxTextureLayoutBinding, irradianceTextureLayoutBinding, prefilteredTextureLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(setBindings.size()), setBindings.data());

    VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayoutSkybox));

    return VK_SUCCESS;
}

VkResult VulkanRendererSkybox::createGraphicsPipeline()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skybox.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/skybox.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsPos();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0, 0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);
    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);

    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 2> descriptorSetsLayouts{m_descriptorSetLayoutCamera, m_descriptorSetLayoutSkybox};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 0, nullptr);

    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    /* Create the graphics pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayout, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}
}  // namespace vengine