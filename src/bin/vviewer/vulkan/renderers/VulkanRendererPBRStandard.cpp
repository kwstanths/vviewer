#include "VulkanRendererPBRStandard.hpp"

#include <debug_tools/Console.hpp>
#include <set>

#include "core/AssetManager.hpp"

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
                                          VkDescriptorSetLayout skyboxDescriptorLayout,
                                          VkDescriptorSetLayout materialDescriptorLayout,
                                          VulkanTextures &textures)
{
    m_physicalDevice = physicalDevice;
    m_device = device;
    m_commandPool = commandPool;
    m_queue = queue;
    m_descriptorSetLayoutCamera = cameraDescriptorLayout;
    m_descriptorSetLayoutModel = modelDescriptorLayout;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;
    m_descriptorSetLayoutMaterial = materialDescriptorLayout;
    m_descriptorSetLayoutTextures = textures.descriptorSetLayout();

    VULKAN_CHECK_CRITICAL(createBRDFLUT(textures));

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::initSwapChainResources(VkExtent2D swapchainExtent,
                                                   VkRenderPass renderPass,
                                                   uint32_t swapchainImages,
                                                   VkSampleCountFlagBits msaaSamples)
{
    m_swapchainExtent = swapchainExtent;
    m_renderPass = renderPass;
    m_msaaSamples = msaaSamples;

    VULKAN_CHECK_CRITICAL(createGraphicsPipelineBasePass());
    VULKAN_CHECK_CRITICAL(createGraphicsPipelineAddPass());

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::releaseSwapChainResources()
{
    vkDestroyPipeline(m_device, m_graphicsPipelineBasePass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutBasePass, nullptr);

    vkDestroyPipeline(m_device, m_graphicsPipelineAddPass, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayoutAddPass, nullptr);

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
    auto vktex = std::make_shared<VulkanTexture>(
        "PBR_BRDF_LUT", TextureType::HDR, format, imageWidth, imageHeight, image, imageMemory, imageView, imageSampler, 0);
    /* Add to resources */
    AssetManager<std::string, Texture> &instance = AssetManager<std::string, Texture>::getInstance();
    instance.add("PBR_BRDF_LUT", vktex);
    /* Add texture to textures descriptor managemenet */
    textures.addTexture(vktex);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::renderObjectsBasePass(VkCommandBuffer &cmdBuf,
                                                  VkDescriptorSet &descriptorScene,
                                                  VkDescriptorSet &descriptorModel,
                                                  VkDescriptorSet descriptorSkybox,
                                                  VkDescriptorSet &descriptorMaterials,
                                                  VkDescriptorSet &descriptorTextures,
                                                  uint32_t imageIndex,
                                                  const VulkanUBO<ModelData> &dynamicUBOModels,
                                                  std::vector<std::shared_ptr<SceneObject>> &objects) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineBasePass);
    for (size_t i = 0; i < objects.size(); i++) {
        VulkanSceneObject *object = static_cast<VulkanSceneObject *>(objects[i].get());

        const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(object->get<ComponentMesh>().mesh.get());
        if (vkmesh == nullptr)
            continue;

        VulkanMaterialPBRStandard *material =
            static_cast<VulkanMaterialPBRStandard *>(object->get<ComponentMaterial>().material.get());

        VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

        /* Calculate model data offsets */
        std::array<uint32_t, 1> dynamicOffsets = {dynamicUBOModels.blockSizeAligned() * object->getTransformUBOBlock()};
        std::array<VkDescriptorSet, 5> descriptorSets = {
            descriptorScene, descriptorModel, descriptorMaterials, descriptorTextures, descriptorSkybox};

        PushBlockForwardBasePass pushConstants;
        pushConstants.selected = glm::vec4(object->getIDRGB(), object->m_isSelected);
        pushConstants.material.r = material->getMaterialIndex();
        vkCmdPushConstants(
            cmdBuf, m_pipelineLayoutBasePass, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockForwardBasePass), &pushConstants);
        vkCmdBindDescriptorSets(cmdBuf,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_pipelineLayoutBasePass,
                                0,
                                static_cast<uint32_t>(descriptorSets.size()),
                                &descriptorSets[0],
                                static_cast<uint32_t>(dynamicOffsets.size()),
                                &dynamicOffsets[0]);

        vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::renderObjectsAddPass(VkCommandBuffer &cmdBuf,
                                                 VkDescriptorSet &descriptorScene,
                                                 VkDescriptorSet &descriptorModel,
                                                 VkDescriptorSet &descriptorMaterials,
                                                 VkDescriptorSet &descriptorTextures,
                                                 uint32_t imageIndex,
                                                 const VulkanUBO<ModelData> &dynamicUBOModels,
                                                 std::shared_ptr<SceneObject> object,
                                                 PushBlockForwardAddPass &lightInfo) const
{
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineAddPass);

    VulkanSceneObject *vobject = static_cast<VulkanSceneObject *>(object.get());
    const VulkanMesh *vkmesh = static_cast<const VulkanMesh *>(vobject->get<ComponentMesh>().mesh.get());
    assert(vkmesh != nullptr);

    VulkanMaterialPBRStandard *material = static_cast<VulkanMaterialPBRStandard *>(vobject->get<ComponentMaterial>().material.get());

    VkBuffer vertexBuffers[] = {vkmesh->vertexBuffer().buffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, vkmesh->indexBuffer().buffer(), 0, vkmesh->indexType());

    /* Calculate model data offsets */
    std::array<uint32_t, 1> dynamicOffsets = {dynamicUBOModels.blockSizeAligned() * vobject->getTransformUBOBlock()};
    /* Create descriptor sets array */
    std::array<VkDescriptorSet, 4> descriptorSets = {descriptorScene, descriptorModel, descriptorMaterials, descriptorTextures};

    lightInfo.material.r = material->getMaterialIndex();

    vkCmdPushConstants(cmdBuf, m_pipelineLayoutAddPass, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlockForwardAddPass), &lightInfo);
    vkCmdBindDescriptorSets(cmdBuf,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_pipelineLayoutAddPass,
                            0,
                            static_cast<uint32_t>(descriptorSets.size()),
                            &descriptorSets[0],
                            static_cast<uint32_t>(dynamicOffsets.size()),
                            &dynamicOffsets[0]);

    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(vkmesh->getIndices().size()), 1, 0, 0, 0);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::createGraphicsPipelineBasePass()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/pbrBase.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0.0, 0.0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 5> descriptorSetsLayouts{m_descriptorSetLayoutCamera,
                                                               m_descriptorSetLayoutModel,
                                                               m_descriptorSetLayoutMaterial,
                                                               m_descriptorSetLayoutTextures,
                                                               m_descriptorSetLayoutSkybox};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForwardBasePass), 0);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutBasePass));

    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutBasePass, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineBasePass));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanRendererPBR::createGraphicsPipelineAddPass()
{
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_VERTEX_BIT, VulkanShader::load(m_device, "shaders/SPIRV/standard.vert.spv"), "main");
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_FRAGMENT_BIT, VulkanShader::load(m_device, "shaders/SPIRV/pbrAdd.frag.spv"), "main");
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription = VulkanVertex::getBindingDescription();
    auto attributeDescriptions = VulkanVertex::getAttributeDescriptionsFull();
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = vkinit::pipelineVertexInputStateCreateInfo(
        1, &bindingDescription, static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = vkinit::pipelineInputAssemblyCreateInfo();

    VkViewport viewport = vkinit::viewport(m_swapchainExtent.width, m_swapchainExtent.height, 0.0F, 1.0F);
    VkRect2D scissor = vkinit::rect2D(m_swapchainExtent.width, m_swapchainExtent.height, 0.0, 0.0);
    VkPipelineViewportStateCreateInfo viewportState = vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);

    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling = vkinit::pipelineMultisampleStateCreateInfo(m_msaaSamples);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_EQUAL);

    VkPipelineColorBlendAttachmentState colorBlendAttachment = vkinit::pipelineColorBlendAttachmentState(
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_TRUE);
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments{colorBlendAttachment, colorBlendAttachment};
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(static_cast<uint32_t>(colorBlendAttachments.size()), colorBlendAttachments.data());

    std::array<VkDescriptorSetLayout, 4> descriptorSetsLayouts{
        m_descriptorSetLayoutCamera, m_descriptorSetLayoutModel, m_descriptorSetLayoutMaterial, m_descriptorSetLayoutTextures};
    VkPushConstantRange pushConstantRange =
        vkinit::pushConstantRange(VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(PushBlockForwardAddPass), 0);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = vkinit::pipelineLayoutCreateInfo(
        static_cast<uint32_t>(descriptorSetsLayouts.size()), descriptorSetsLayouts.data(), 1, &pushConstantRange);
    VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayoutAddPass));

    /* Create the graphics pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo = vkinit::graphicsPipelineCreateInfo(m_pipelineLayoutAddPass, m_renderPass, 0);
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;

    VULKAN_CHECK_CRITICAL(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelineAddPass));

    vkDestroyShaderModule(m_device, fragShaderStageInfo.module, nullptr);
    vkDestroyShaderModule(m_device, vertShaderStageInfo.module, nullptr);

    return VK_SUCCESS;
}

}  // namespace vengine