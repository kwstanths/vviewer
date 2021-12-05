#ifndef __VulkanUtils_hpp__
#define __VulkanUtils_hpp__

#include <vector>
#include <fstream>
#include <array>

#include <glm/glm.hpp>
#include "core/Mesh.hpp"
#include "vulkan/IncludeVulkan.hpp"

/**

*/
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

/**

*/
bool createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags bufferProperties, VkBuffer & buffer, VkDeviceMemory & bufferMemory);

/**

*/
bool createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, const std::vector<Vertex>& vertices, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

/**

*/
bool createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, const std::vector<uint16_t>& indices, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

/**

*/
bool createImage(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

/**

*/
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

/**

*/
bool copyBufferToBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

/**

*/
bool copyBufferToImage(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer buffer, VkImage image, std::vector<VkBufferImageCopy> regions);

/**

*/
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

/**

*/
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer);

/**
    Submit a command to transition the layout of an image from oldLayout to newLayout
*/
void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t nLayers = 1);
void transitionImageLayout(VkCommandBuffer cmdBuf, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t nLayers = 1);

/**
    
*/
std::vector<char> readSPIRV(const std::string& filename);

#endif