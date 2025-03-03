#include "VulkanUtils.hpp"

#include <cstdint>
#include <unordered_map>

#include <debug_tools/Console.hpp>

#include "VulkanInitializers.hpp"

namespace vengine
{

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
                             const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    debug_tools::ConsoleCritical("VulkanSwapchain::findSupportedFormat(): Failed to find supported format");
    return VK_FORMAT_UNDEFINED;
}

std::vector<VulkanQueueFamilyInfo> findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    std::vector<VulkanQueueFamilyInfo> families;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) 
    {
        VulkanQueueFamilyInfo info;
        info.index = i;
        info.properties = queueFamily;
        info.present = false;

        // Present support
        if (surface != VK_NULL_HANDLE) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                info.present = true;
            }
        }

        i++;
        families.push_back(info);
    }

    return families;
}

VulkanSwapChainDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    VulkanSwapChainDetails details;

    /* Query surface capabilities */
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    /* Query formats */
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    /* Query presentation modes */
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    debug_tools::ConsoleFatal("Failed to find a suitable memory type");

    return 0;
}

VkResult createBuffer(VkPhysicalDevice physicalDevice,
                      VkDevice device,
                      VkDeviceSize bufferSize,
                      VkBufferUsageFlags bufferUsage,
                      VkMemoryPropertyFlags bufferProperties,
                      VulkanBuffer &outBuffer)
{
    if (bufferSize <= 0) {
        debug_tools::ConsoleWarning("createBuffer(): Trying to allocate a buffer with zero size");
        return VK_ERROR_UNKNOWN;
    }

    /* Create buffer */
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = bufferUsage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VULKAN_CHECK_CRITICAL(vkCreateBuffer(device, &bufferInfo, nullptr, &outBuffer.buffer()));

    /* Get buffer memory requirements */
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, outBuffer.buffer(), &memoryRequirements);

    /* Alocate memory for buffer */
    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, bufferProperties);

    /* If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation */
    VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
    if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        allocateInfo.pNext = &allocFlagsInfo;
    }

    VULKAN_CHECK_CRITICAL(vkAllocateMemory(device, &allocateInfo, nullptr, &outBuffer.memory()));

    /* Bind memory to buffer */
    VULKAN_CHECK_CRITICAL(vkBindBufferMemory(device, outBuffer.buffer(), outBuffer.memory(), 0));

    /* Set the buffer device address if usage is requested s*/
    if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        outBuffer.address() = getBufferDeviceAddress(device, outBuffer.buffer());
    }

    return VK_SUCCESS;
}

VkResult createBuffer(VkPhysicalDevice physicalDevice,
                      VkDevice device,
                      VkDeviceSize bufferSize,
                      VkBufferUsageFlags bufferUsage,
                      VkMemoryPropertyFlags bufferProperties,
                      const void *data,
                      VulkanBuffer &outBuffer)
{
    VULKAN_CHECK_CRITICAL(createBuffer(physicalDevice, device, bufferSize, bufferUsage, bufferProperties, outBuffer));

    /* copy data */
    void *mapped;
    VULKAN_CHECK_CRITICAL(vkMapMemory(device, outBuffer.memory(), 0, bufferSize, 0, &mapped));
    memcpy(mapped, data, bufferSize);
    vkUnmapMemory(device, outBuffer.memory());

    return VK_SUCCESS;
}

VkResult createVertexBuffer(VulkanCommandInfo vci,
                            const std::vector<Vertex> &vertices,
                            VkBufferUsageFlags extraUsageFlags,
                            VulkanBuffer &outBuffer)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VulkanBuffer stagingBuffer;
    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       bufferSize,
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       stagingBuffer));

    void *data;
    VULKAN_CHECK_CRITICAL(vkMapMemory(vci.device, stagingBuffer.memory(), 0, bufferSize, 0, &data));
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(vci.device, stagingBuffer.memory());

    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       bufferSize,
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | extraUsageFlags,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       outBuffer));

    VULKAN_CHECK_CRITICAL(copyBufferToBuffer(vci, stagingBuffer.buffer(), outBuffer.buffer(), bufferSize));

    stagingBuffer.destroy(vci.device);

    return VK_SUCCESS;
}

VkResult createIndexBuffer(VulkanCommandInfo vci,
                           const std::vector<uint32_t> &indices,
                           VkBufferUsageFlags extraUsageFlags,
                           VulkanBuffer &outBuffer)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VulkanBuffer stagingBuffer;
    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       bufferSize,
                                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       stagingBuffer));

    void *data;
    VULKAN_CHECK_CRITICAL(vkMapMemory(vci.device, stagingBuffer.memory(), 0, bufferSize, 0, &data));
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vci.device, stagingBuffer.memory());

    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       bufferSize,
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | extraUsageFlags,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       outBuffer));

    VULKAN_CHECK_CRITICAL(copyBufferToBuffer(vci, stagingBuffer.buffer(), outBuffer.buffer(), bufferSize));

    stagingBuffer.destroy(vci.device);

    return VK_SUCCESS;
}

VkResult createImage(VkPhysicalDevice physicalDevice,
                     VkDevice device,
                     const VkImageCreateInfo &imageCreateInfo,
                     const VkMemoryPropertyFlags &properties,
                     VkImage &image,
                     VkDeviceMemory &imageMemory)
{
    VULKAN_CHECK_CRITICAL(vkCreateImage(device, &imageCreateInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    VULKAN_CHECK_CRITICAL(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory));

    VULKAN_CHECK_CRITICAL(vkBindImageMemory(device, image, imageMemory, 0));

    return VK_SUCCESS;
}

VkResult copyBufferToBuffer(VulkanCommandInfo vci, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    /* Create command buffer for transfer operation */
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    /* Record transfer command */
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /* End recording and submit */
    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    return VK_SUCCESS;
}

VkResult copyBufferToImage(VulkanCommandInfo vci, VkBuffer buffer, VkImage image, std::vector<VkBufferImageCopy> regions)
{
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    vkCmdCopyBufferToImage(
        commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    return VK_SUCCESS;
}

VkResult beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer &commandBuffer)
{
    VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool, 1);
    VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer));

    /* Start recording commands */
    VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    return VK_SUCCESS;
}

VkResult endSingleTimeCommands(VkDevice device,
                               VkCommandPool commandPool,
                               VkQueue queue,
                               VkCommandBuffer commandBuffer,
                               bool freeCommandBuffer,
                               uint64_t timeout)
{
    VULKAN_CHECK_CRITICAL(vkEndCommandBuffer(commandBuffer));

    /* Create fence to wait for the command to finish */
    VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo();
    VkFence fence;
    VULKAN_CHECK_CRITICAL(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));

    /* Submit command buffer */
    VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);
    VULKAN_CHECK_CRITICAL(vkQueueSubmit(queue, 1, &submitInfo, fence));

    // Wait for the fence to signal that command buffer has finished executing
    VkResult ret = vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
    VULKAN_CHECK_CRITICAL(ret);

    if (ret == VK_SUCCESS) {
        vkDestroyFence(device, fence, nullptr);
        if (freeCommandBuffer) {
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }
    }

    return ret;
}

VkResult transitionImageLayout(VulkanCommandInfo vci,
                               VkImage image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               uint32_t numMips,
                               uint32_t nLayers)
{
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    transitionImageLayout(commandBuffer, image, oldLayout, newLayout, numMips, nLayers);

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    return VK_SUCCESS;
}

void transitionImageLayout(VkCommandBuffer cmdBuf,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           uint32_t numMips,
                           uint32_t nLayers)
{
    VkImageSubresourceRange resourceRange = {};
    resourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    resourceRange.baseMipLevel = 0;
    resourceRange.levelCount = numMips;
    resourceRange.baseArrayLayer = 0;
    resourceRange.layerCount = nLayers;

    transitionImageLayout(cmdBuf, image, oldLayout, newLayout, resourceRange);
}

VkResult transitionImageLayout(VulkanCommandInfo vci,
                               VkImage image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkImageSubresourceRange resourceRange)
{
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    transitionImageLayout(commandBuffer, image, oldLayout, newLayout, resourceRange);

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    return VK_SUCCESS;
}

void transitionImageLayout(VkCommandBuffer cmdBuf,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkImageSubresourceRange resourceRange)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = resourceRange;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    switch (oldLayout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            // Image layout is undefined (or does not matter)
            // Only valid as initial layout
            // No flags required, listed only for completeness
            barrier.srcAccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            // Image is preinitialized
            // Only valid as initial layout for linear images, preserves memory contents
            // Make sure host writes have been finished
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image is a color attachment
            // Make sure any writes to the color buffer have been finished
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image is a depth/stencil attachment
            // Make sure any writes to the depth/stencil buffer have been finished
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image is a transfer source
            // Make sure any reads from the image have been finished
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image is a transfer destination
            // Make sure any writes to the image have been finished
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image is read by a shader
            // Make sure any shader reads from the image have been finished
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
    }

    // Target layouts (new)
    // Destination access mask controls the dependency for the new image layout
    switch (newLayout) {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            // Image will be used as a transfer destination
            // Make sure any writes to the image have been finished
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            // Image will be used as a transfer source
            // Make sure any reads from the image have been finished
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            // Image will be used as a color attachment
            // Make sure any writes to the color buffer have been finished
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            // Image layout will be used as a depth/stencil attachment
            // Make sure any writes to depth/stencil buffer have been finished
            barrier.dstAccessMask = barrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            // Image will be read in a shader (sampler, input attachment)
            // Make sure any writes to the image have been finished
            if (barrier.srcAccessMask == 0) {
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            // Other source layouts aren't handled (yet)
            break;
    }

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmdBuf, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat)
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT,
                                          VK_FORMAT_D32_SFLOAT,
                                          VK_FORMAT_D24_UNORM_S8_UINT,
                                          VK_FORMAT_D16_UNORM_S8_UINT,
                                          VK_FORMAT_D16_UNORM};

    for (auto &format : depthFormats) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

VkFormat getVkFormat(Format format)
{
    switch (format) {
        case vengine::Format::UNKNOWN:
            return VK_FORMAT_UNDEFINED;
        case vengine::Format::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case vengine::Format::R32_SFLOAT:
            return VK_FORMAT_R32_SFLOAT;
        case vengine::Format::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case vengine::Format::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case vengine::Format::R32G32B32A32_SFLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case vengine::Format::R16G16_SFLOAT:
            return VK_FORMAT_R16G16_SFLOAT;
        case vengine::Format::BC1_RGB_UNORM_BLOCK:
            return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case vengine::Format::BC1_RGBA_UNORM_BLOCK:
            return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case vengine::Format::BC3_UNORM_BLOCK:
            return VK_FORMAT_BC3_UNORM_BLOCK;
        case vengine::Format::BC3_SRGB_BLOCK:
            return VK_FORMAT_BC3_SRGB_BLOCK;
        case vengine::Format::BC5_UNORM:
            return VK_FORMAT_BC5_UNORM_BLOCK;
        default:
            return VK_FORMAT_UNDEFINED;
    }
    return VK_FORMAT_UNDEFINED;
}

VkDeviceOrHostAddressKHR getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;

    VkDeviceOrHostAddressKHR adress;
    adress.deviceAddress = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
        vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"))(device, &bufferDeviceAI);
    return adress;
}

}  // namespace vengine
