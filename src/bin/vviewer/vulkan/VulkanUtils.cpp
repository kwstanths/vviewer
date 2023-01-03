#include "VulkanUtils.hpp"

#include <cstdint>
#include <utils/Console.hpp>
#include <vulkan/vulkan_core.h>

uint32_t findMemoryType(VkPhysicalDevice physicalDevice,
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    utils::ConsoleFatal("Failed to find a suitable memory type");

    return 0;
}

bool createBuffer(VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize bufferSize,
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags bufferProperties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    if (bufferSize <= 0)
    {
        utils::ConsoleWarning("createBuffer(): Trying to allocate a buffer with zero size");
        return false;
    }

    /* Create vertex buffer */
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = bufferUsage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (res != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a vertex buffer");
        return false;
    }

    /* Get buffer memory requirements */
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

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

    res = vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory);
    if (res != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate device memory: " + std::to_string(res));
        return false;
    }
    /* Bind memory to buffer */
    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    return true;
}

bool createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, const void* data, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    bool ret = createBuffer(physicalDevice, device, bufferSize, bufferUsage, bufferProperties, buffer, bufferMemory);

    if (!ret) return ret;

    void* mapped;
    VkResult res = vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &mapped);
    if (res != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to map device memory: " + std::to_string(res));
        return false;
    }

    memcpy(mapped, data, bufferSize);
    
    /* If host coherency hasn't been requested, do a manual flush to make writes visible */
    if ((bufferProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        VkMappedMemoryRange mappedMemoryRange{};
        mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        mappedMemoryRange.memory = bufferMemory;
        mappedMemoryRange.offset = 0;
        mappedMemoryRange.size = bufferSize;
        vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
    }
    vkUnmapMemory(device, bufferMemory);

    return VK_SUCCESS;
}

bool createVertexBuffer(VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkQueue transferQueue,
    VkCommandPool transferCommandPool,
    const std::vector<Vertex>& vertices,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer,
        bufferMemory
    );

    copyBufferToBuffer(device, transferQueue, transferCommandPool, stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return true;
}

bool createIndexBuffer(VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkQueue transferQueue,
    VkCommandPool transferCommandPool,
    const std::vector<uint32_t>& indices,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(physicalDevice, device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(physicalDevice, device, bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer,
        bufferMemory);

    copyBufferToBuffer(device, transferQueue, transferCommandPool, stagingBuffer, buffer, bufferSize);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    return true;
}

bool createImage(VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    uint32_t width, 
    uint32_t height, 
    uint32_t numMips,
    VkSampleCountFlagBits numSamples,
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags properties, 
    VkImage & image, 
    VkDeviceMemory & imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = numMips;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a vulkan image");
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate memory for a vulkan image");
        return false;
    }

    vkBindImageMemory(device, image, imageMemory, 0);
    return true;
}

VkImageView createImageView(VkDevice device, 
    VkImage image, 
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    uint32_t numMips)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = numMips;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create an image view");
        return imageView;
    }

    return imageView;
}

bool copyBufferToBuffer(VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    VkBuffer srcBuffer, 
    VkBuffer dstBuffer, 
    VkDeviceSize size)
{
    /* Create command buffer for transfer operation */
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, transferCommandPool);

    /* Record transfer command */
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    /* End recording and submit */
    endSingleTimeCommands(device, transferCommandPool, transferQueue, commandBuffer);

    return true;
}

bool copyBufferToImage(VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    VkBuffer buffer, 
    VkImage image, 
    std::vector<VkBufferImageCopy> regions)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, transferCommandPool);

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(regions.size()),
        regions.data()
    );

    endSingleTimeCommands(device, transferCommandPool, transferQueue, commandBuffer);
    return true;
}

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    /* Start recording commands */
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, 
    VkCommandPool commandPool, 
    VkQueue queue, 
    VkCommandBuffer commandBuffer,
    bool freeCommandBuffer,
    uint64_t timeout)
{
    vkEndCommandBuffer(commandBuffer);

    /* Create fence to wait for the command to finish */
    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;
    VkFence fence;
    vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

    /* Submit command buffer */
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(queue, 1, &submitInfo, fence);
    
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device, 1, &fence, VK_TRUE, timeout);
    vkDestroyFence(device, fence, nullptr);
    if (freeCommandBuffer)
    {
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }
}

void transitionImageLayout(VkDevice device, 
    VkQueue queue, 
    VkCommandPool commandPool, 
    VkImage image, 
    VkImageLayout oldLayout, 
    VkImageLayout newLayout, 
    uint32_t numMips, 
    uint32_t nLayers)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    transitionImageLayout(commandBuffer, image, oldLayout, newLayout, numMips, nLayers);

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);
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

void transitionImageLayout(VkDevice device, 
    VkQueue queue, 
    VkCommandPool commandPool, 
    VkImage image, 
    VkImageLayout oldLayout, 
    VkImageLayout newLayout, 
    VkImageSubresourceRange resourceRange)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

    transitionImageLayout(commandBuffer, image, oldLayout, newLayout, resourceRange);

    endSingleTimeCommands(device, commandPool, queue, commandBuffer);
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

    switch (oldLayout)
    {
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
    switch (newLayout)
    {
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
        if (barrier.srcAccessMask == 0)
        {
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

    vkCmdPipelineBarrier(
        cmdBuf,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<VkFormat> depthFormats = {
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

void createAccelerationStructureBuffer(VkPhysicalDevice physicalDevice, 
    VkDevice device,
    VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, 
    VkAccelerationStructureKHR& handle, 
    uint64_t& deviceAddress, 
    VkDeviceMemory& memory, 
    VkBuffer& buffer)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkResult res = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    if (res != VK_SUCCESS)
    {
        utils::ConsoleCritical("Failed to create a buffer for an acceleration sturcture buffer: " + std::to_string(res));
        return;
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    res = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory);
    if (res != VK_SUCCESS)
    {
        utils::ConsoleCritical("Failed to allocate memory for an acceleration sturcture buffer: " + std::to_string(res));
    }

    vkBindBufferMemory(device, buffer, memory, 0);
}

std::vector<char> readSPIRV(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        utils::ConsoleFatal("Failed to parse shader: " + filename);
        exit(-1);
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}
