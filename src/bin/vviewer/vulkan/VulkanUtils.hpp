#ifndef __VulkanUtils_hpp__
#define __VulkanUtils_hpp__

#include <vector>
#include <fstream>
#include <array>
#include <iostream>

#include <glm/glm.hpp>
#include "core/Mesh.hpp"
#include "vulkan/IncludeVulkan.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) 
    {
        //std::cerr << "Debug callback: " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

inline uint32_t alignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
    Find a memory type to be used with VkMemoryAllocateInfo
*/
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, 
    uint32_t typeFilter, 
    VkMemoryPropertyFlags properties);

/**
    Create a buffer
*/
bool createBuffer(VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    VkDeviceSize bufferSize, 
    VkBufferUsageFlags bufferUsage, 
    VkMemoryPropertyFlags bufferProperties, 
    VkBuffer & buffer, 
    VkDeviceMemory & bufferMemory);

/**
    Create a buffer and copy data
*/
bool createBuffer(VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkDeviceSize bufferSize,
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags bufferProperties,
    const void* data,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory);

/**
    Create a vertex buffer
*/
bool createVertexBuffer(VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    const std::vector<Vertex>& vertices, 
    VkBuffer& buffer, 
    VkDeviceMemory& bufferMemory);

/**
    Create an indices buffer
*/
bool createIndexBuffer(VkPhysicalDevice physicalDevice, 
    VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    const std::vector<uint32_t>& indices, 
    VkBuffer& buffer, 
    VkDeviceMemory& bufferMemory);

/**
    Create an Image
*/
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
    VkImage& image, 
    VkDeviceMemory& imageMemory);

/**
    Create an image view
*/
VkImageView createImageView(VkDevice device, 
    VkImage image, 
    VkFormat format, 
    VkImageAspectFlags aspectFlags, 
    uint32_t numMips);

/**
    Copy a buffer to an another buffer
*/
bool copyBufferToBuffer(VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    VkBuffer srcBuffer, 
    VkBuffer dstBuffer, 
    VkDeviceSize size);

/**
    Copy a buffer to an image
*/
bool copyBufferToImage(VkDevice device, 
    VkQueue transferQueue, 
    VkCommandPool transferCommandPool, 
    VkBuffer buffer, 
    VkImage image, 
    std::vector<VkBufferImageCopy> regions);


VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, 
    VkCommandPool commandPool, 
    VkQueue queue, 
    VkCommandBuffer commandBuffer,
    bool freeCommandBuffer = true);

/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, creates and destroys a command buffer 
*/
void transitionImageLayout(VkDevice device, 
    VkQueue queue, 
    VkCommandPool commandPool, 
    VkImage image, 
    VkImageLayout oldLayout, 
    VkImageLayout newLayout, 
    uint32_t numMips,
    uint32_t nLayers = 1);
/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, uses an existing command buffer 
*/
void transitionImageLayout(VkCommandBuffer cmdBuf, 
    VkImage image, 
    VkImageLayout oldLayout, 
    VkImageLayout newLayout, 
    uint32_t numMips,
    uint32_t nLayers = 1);
/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, using a custom resource range for the image
    creates and destroys a command buffer
*/
void transitionImageLayout(VkDevice device,
    VkQueue queue,
    VkCommandPool commandPool,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange resourceRange);
/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, using a custom resource range for the image
    uses an existing command buffer
*/
void transitionImageLayout(
    VkCommandBuffer cmdBuf,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange resourceRange);

/**

*/
VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat);

/**

*/
void createAccelerationStructureBuffer(VkPhysicalDevice physicalDevice, 
    VkDevice device,
    VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, 
    VkAccelerationStructureKHR& handle, 
    uint64_t& deviceAddress,
    VkDeviceMemory& memory,
    VkBuffer& buffer);

/**
    
*/
std::vector<char> readSPIRV(const std::string& filename);

#endif