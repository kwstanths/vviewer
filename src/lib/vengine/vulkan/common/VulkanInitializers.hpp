#ifndef __VulkanInitializers_hpp__
#define __VulkanInitializers_hpp__

#include "IncludeVulkan.hpp"

namespace vengine::vkinit
{

inline VkViewport viewport(float width, float height, float minDepth, float maxDepth, float x = 0.0F, float y = 0.0F)
{
    VkViewport viewport{};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = minDepth;
    viewport.maxDepth = maxDepth;
    viewport.x = x;
    viewport.y = y;
    return viewport;
}

inline VkRect2D rect2D(int32_t width, int32_t height, int32_t offsetX, int32_t offsetY)
{
    VkRect2D rect2D{};
    rect2D.extent.width = width;
    rect2D.extent.height = height;
    rect2D.offset.x = offsetX;
    rect2D.offset.y = offsetY;
    return rect2D;
}

inline VkImageCreateInfo imageCreateInfo(VkExtent3D extent,
                                         VkFormat format,
                                         uint32_t mipLevels,
                                         VkSampleCountFlagBits samples,
                                         VkImageTiling tiling,
                                         VkImageUsageFlags usage)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.samples = samples;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.extent = extent;
    imageCreateInfo.usage = usage;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.flags = 0;
    return imageCreateInfo;
}

inline VkImageViewCreateInfo imageViewCreateInfo(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t numMips)
{
    VkImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = numMips;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;
    return imageViewCreateInfo;
}

inline VkSamplerCreateInfo samplerCreateInfo(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode)
{
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = magFilter;
    samplerCreateInfo.minFilter = minFilter;
    samplerCreateInfo.mipmapMode = mipmapMode;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 1.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 0.0f;
    return samplerCreateInfo;
}

inline VkSubmitInfo submitInfo(uint32_t commandBufferCount, const VkCommandBuffer *commandBuffers)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = commandBufferCount;
    submitInfo.pCommandBuffers = commandBuffers;
    return submitInfo;
}

inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0)
{
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = flags;
    return fenceCreateInfo;
}

/**
 * @brief Create a VkAttachmentDescription
 *
 * @param attachmentFormat That format that will be used for this image attachment
 * @param samples Number of samples on the image
 * @param loadOp What should happen to this attachment image at the beginning of the subpass where it's first used
 * @param storeOp What should happen to this attachment image at the end of the subpass where it's last used
 * @param initialLayout The layout at which the attachment image will be at the beggining of the render pass
 * @param finalLayout The layout at which the attachment image will be at the end of the render pass
 * @return VkAttachmentDescription
 */
inline VkAttachmentDescription attachmentDescriptionInfo(VkFormat attachmentFormat,
                                                         VkSampleCountFlagBits samples,
                                                         VkAttachmentLoadOp loadOp,
                                                         VkAttachmentStoreOp storeOp,
                                                         VkImageLayout initialLayout,
                                                         VkImageLayout finalLayout)
{
    VkAttachmentDescription attachment{};
    attachment.format = attachmentFormat;
    attachment.samples = samples;
    attachment.loadOp = loadOp;
    attachment.storeOp = storeOp;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = initialLayout;
    attachment.finalLayout = finalLayout;
    return attachment;
}

inline VkRenderPassCreateInfo renderPassCreateInfo(uint32_t attachmentCount,
                                                   const VkAttachmentDescription *attachments,
                                                   uint32_t subpassCount,
                                                   const VkSubpassDescription *subpasses,
                                                   uint32_t dependencyCount,
                                                   const VkSubpassDependency *subpassDependencies)
{
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = attachmentCount;
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = subpassCount;
    renderPassCreateInfo.pSubpasses = subpasses;
    renderPassCreateInfo.dependencyCount = dependencyCount;
    renderPassCreateInfo.pDependencies = subpassDependencies;
    return renderPassCreateInfo;
}

inline VkFramebufferCreateInfo framebufferCreateInfo(VkRenderPass renderPass,
                                                     uint32_t attachmentCount,
                                                     const VkImageView *attachments,
                                                     uint32_t width,
                                                     uint32_t height)
{
    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderPass;
    framebufferCreateInfo.attachmentCount = attachmentCount;
    framebufferCreateInfo.pAttachments = attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;
    return framebufferCreateInfo;
}

inline VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
{
    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type = type;
    descriptorPoolSize.descriptorCount = descriptorCount;
    return descriptorPoolSize;
}

inline VkDescriptorPoolCreateInfo descriptorPoolCreateInfo(uint32_t poolSizeCount, VkDescriptorPoolSize *pPoolSizes, uint32_t maxSets)
{
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = poolSizeCount;
    descriptorPoolInfo.pPoolSizes = pPoolSizes;
    descriptorPoolInfo.maxSets = maxSets;
    return descriptorPoolInfo;
}

inline VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(VkDescriptorType type,
                                                               VkShaderStageFlags stageFlags,
                                                               uint32_t binding,
                                                               uint32_t descriptorCount = 1)
{
    VkDescriptorSetLayoutBinding setLayoutBinding{};
    setLayoutBinding.descriptorType = type;
    setLayoutBinding.stageFlags = stageFlags;
    setLayoutBinding.binding = binding;
    setLayoutBinding.descriptorCount = descriptorCount;
    setLayoutBinding.pImmutableSamplers = nullptr;
    return setLayoutBinding;
}

inline VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(uint32_t bindingCount,
                                                                     const VkDescriptorSetLayoutBinding *pBindings)
{
    VkDescriptorSetLayoutCreateInfo descriptorsetlayoutCreateInfo = {};
    descriptorsetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorsetlayoutCreateInfo.bindingCount = bindingCount;
    descriptorsetlayoutCreateInfo.pBindings = pBindings;
    return descriptorsetlayoutCreateInfo;
}

inline VkDescriptorSetAllocateInfo descriptorSetAllocateInfo(VkDescriptorPool descriptorPool,
                                                             uint32_t descriptorSetCount,
                                                             const VkDescriptorSetLayout *pSetLayouts)
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = pSetLayouts;
    descriptorSetAllocateInfo.descriptorSetCount = descriptorSetCount;
    return descriptorSetAllocateInfo;
}

inline VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
    VkDescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.buffer = buffer;
    descriptorBufferInfo.offset = offset;
    descriptorBufferInfo.range = range;
    return descriptorBufferInfo;
}

inline VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
{
    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.sampler = sampler;
    descriptorImageInfo.imageView = imageView;
    descriptorImageInfo.imageLayout = imageLayout;
    return descriptorImageInfo;
}

inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet dstSet,
                                               VkDescriptorType type,
                                               uint32_t binding,
                                               uint32_t descriptorCount,
                                               const VkDescriptorBufferInfo *bufferInfo)
{
    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pBufferInfo = bufferInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

inline VkWriteDescriptorSet writeDescriptorSet(VkDescriptorSet dstSet,
                                               VkDescriptorType type,
                                               uint32_t binding,
                                               uint32_t descriptorCount,
                                               const VkDescriptorImageInfo *imageInfo)
{
    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = dstSet;
    writeDescriptorSet.descriptorType = type;
    writeDescriptorSet.dstBinding = binding;
    writeDescriptorSet.pImageInfo = imageInfo;
    writeDescriptorSet.descriptorCount = descriptorCount;
    return writeDescriptorSet;
}

inline VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo(uint32_t setLayoutCount,
                                                           const VkDescriptorSetLayout *setLayouts,
                                                           uint32_t pushConstantRangeCount,
                                                           const VkPushConstantRange *pushConstants)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = setLayoutCount;
    pipelineLayoutCreateInfo.pSetLayouts = setLayouts;
    pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants;
    return pipelineLayoutCreateInfo;
}

inline VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyCreateInfo(
    VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
{
    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyCreateInfo{};
    pipelineInputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyCreateInfo.topology = primitiveTopology;
    pipelineInputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;
    return pipelineInputAssemblyCreateInfo;
}

inline VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode,
                                                                                   VkCullModeFlags cullMode,
                                                                                   VkFrontFace frontFace,
                                                                                   VkPipelineRasterizationStateCreateFlags flags = 0)
{
    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
    pipelineRasterizationStateCreateInfo.cullMode = cullMode;
    pipelineRasterizationStateCreateInfo.frontFace = frontFace;
    pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.flags = flags;
    return pipelineRasterizationStateCreateInfo;
}

inline VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask,
                                                                             VkBool32 blendEnable)
{
    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
    pipelineColorBlendAttachmentState.colorWriteMask = colorWriteMask;
    pipelineColorBlendAttachmentState.blendEnable = blendEnable;
    return pipelineColorBlendAttachmentState;
}

inline VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo(uint32_t attachmentCount,
                                                                             const VkPipelineColorBlendAttachmentState *attachments)
{
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    pipelineColorBlendStateCreateInfo.attachmentCount = attachmentCount;
    pipelineColorBlendStateCreateInfo.pAttachments = attachments;
    return pipelineColorBlendStateCreateInfo;
}

inline VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo(VkBool32 depthTestEnable,
                                                                                 VkBool32 depthWriteEnable,
                                                                                 VkCompareOp depthCompareOp)
{
    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = depthTestEnable;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = depthWriteEnable;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = depthCompareOp;
    pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
    pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
    return pipelineDepthStencilStateCreateInfo;
}

inline VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo(uint32_t viewportCount,
                                                                         const VkViewport *viewports,
                                                                         uint32_t scissorCount,
                                                                         const VkRect2D *scissors,
                                                                         VkPipelineViewportStateCreateFlags flags = 0)
{
    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount = viewportCount;
    pipelineViewportStateCreateInfo.pViewports = viewports;
    pipelineViewportStateCreateInfo.scissorCount = scissorCount;
    pipelineViewportStateCreateInfo.pScissors = scissors;
    pipelineViewportStateCreateInfo.flags = flags;
    return pipelineViewportStateCreateInfo;
}

inline VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo(VkSampleCountFlagBits rasterizationSamples,
                                                                               VkPipelineMultisampleStateCreateFlags flags = 0)
{
    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
    pipelineMultisampleStateCreateInfo.flags = flags;
    return pipelineMultisampleStateCreateInfo;
}

inline VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo(uint32_t dynamicStateCount,
                                                                       const VkDynamicState *dynamicStates,
                                                                       VkPipelineDynamicStateCreateFlags flags = 0)
{
    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
    pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateCount;
    pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates;
    pipelineDynamicStateCreateInfo.flags = 0;
    return pipelineDynamicStateCreateInfo;
}

inline VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
                                                                     VkShaderModule module,
                                                                     const char *entry)
{
    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
    pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageCreateInfo.stage = stage;
    pipelineShaderStageCreateInfo.module = module;
    pipelineShaderStageCreateInfo.pName = entry;
    return pipelineShaderStageCreateInfo;
}

inline VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR(VkRayTracingShaderGroupTypeKHR type,
                                                                               uint32_t generalShader,
                                                                               uint32_t closestHitShader,
                                                                               uint32_t anyHitShader = VK_SHADER_UNUSED_KHR,
                                                                               uint32_t intersectionShader = VK_SHADER_UNUSED_KHR)
{
    VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfoKHR{};
    rayTracingShaderGroupCreateInfoKHR.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rayTracingShaderGroupCreateInfoKHR.type = type;
    rayTracingShaderGroupCreateInfoKHR.generalShader = generalShader;
    rayTracingShaderGroupCreateInfoKHR.closestHitShader = closestHitShader;
    rayTracingShaderGroupCreateInfoKHR.anyHitShader = anyHitShader;
    rayTracingShaderGroupCreateInfoKHR.intersectionShader = intersectionShader;
    return rayTracingShaderGroupCreateInfoKHR;
}

inline VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo(
    uint32_t vertexBindingDescriptionCount,
    const VkVertexInputBindingDescription *vertexBindingDescriptions,
    uint32_t vertexAttributeDescriptionCount,
    const VkVertexInputAttributeDescription *vertexAttributeDescriptions)
{
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexBindingDescriptions;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;
    return pipelineVertexInputStateCreateInfo;
}

inline VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo(VkPipelineLayout layout,
                                                               VkRenderPass renderPass,
                                                               uint32_t subpass,
                                                               VkPipelineCreateFlags flags = 0)
{
    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.layout = layout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = subpass;
    graphicsPipelineCreateInfo.flags = flags;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    return graphicsPipelineCreateInfo;
}

inline VkRenderPassBeginInfo renderPassBeginInfo(VkRenderPass renderPass,
                                                 VkFramebuffer framebuffer,
                                                 uint32_t clearValueCount,
                                                 const VkClearValue *clearValues)
{
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.clearValueCount = clearValueCount;
    renderPassBeginInfo.pClearValues = clearValues;
    return renderPassBeginInfo;
}

inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(VkCommandBufferLevel level,
                                                             VkCommandPool commandPool,
                                                             uint32_t commandBufferCount)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.level = level;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
    return commandBufferAllocateInfo;
}

inline VkCommandBufferBeginInfo commandBufferBeginInfo()
{
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    return commandBufferBeginInfo;
}

inline VkPushConstantRange pushConstantRange(VkShaderStageFlags stageFlags, uint32_t size, uint32_t offset)
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = stageFlags;
    pushConstantRange.size = size;
    pushConstantRange.offset = offset;
    return pushConstantRange;
}

}  // namespace vengine::vkinit

#endif