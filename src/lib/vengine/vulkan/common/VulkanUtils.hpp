#ifndef __VulkanUtils_hpp__
#define __VulkanUtils_hpp__

#include <cstdint>
#include <vector>
#include <fstream>
#include <array>
#include <iostream>

#include <glm/glm.hpp>

#include "core/Mesh.hpp"
#include "core/Color.hpp"
#include "IncludeVulkan.hpp"
#include "VulkanStructs.hpp"
#include "vulkan/resources/VulkanBuffer.hpp"

namespace vengine
{

/* vulkan timeouts in nanoseconds */
static const uint64_t VULKAN_TIMEOUT_100MS = 100000000;
static const uint64_t VULKAN_TIMEOUT_1S = 1000000000;
static const uint64_t VULKAN_TIMEOUT_10S = 10000000000;
static const uint64_t VULKAN_TIMEOUT_100S = 100000000000;

inline uint32_t alignedSize(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice,
                             const std::vector<VkFormat> &candidates,
                             VkImageTiling tiling,
                             VkFormatFeatureFlags features);

/**
 * @brief Find the queue families for a physical device
 *
 * @param device
 * @return
 */
std::vector<VulkanQueueFamilyInfo> findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief Get supported swapchain information
 *
 * @param device
 * @return SwapChainDetails
 */
VulkanSwapChainDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

/**
 * @brief
 *
 * @param physicalDevice
 * @param typeFilter
 * @param properties
 * @return uint32_t
 */
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkResult createBuffer(VkPhysicalDevice physicalDevice,
                      VkDevice device,
                      VkDeviceSize bufferSize,
                      VkBufferUsageFlags bufferUsage,
                      VkMemoryPropertyFlags bufferProperties,
                      VulkanBuffer &outBuffer);

VkResult createBuffer(VkPhysicalDevice physicalDevice,
                      VkDevice device,
                      VkDeviceSize bufferSize,
                      VkBufferUsageFlags bufferUsage,
                      VkMemoryPropertyFlags bufferProperties,
                      const void *data,
                      VulkanBuffer &outBuffer);

VkResult createVertexBuffer(VulkanCommandInfo vci,
                            const std::vector<Vertex> &vertices,
                            VkBufferUsageFlags extraUsageFlags,
                            VulkanBuffer &outBuffer);

VkResult createIndexBuffer(VulkanCommandInfo vci,
                           const std::vector<uint32_t> &indices,
                           VkBufferUsageFlags extraUsageFlags,
                           VulkanBuffer &outBuffer);

VkResult createImage(VkPhysicalDevice physicalDevice,
                     VkDevice device,
                     const VkImageCreateInfo &imageCreateInfo,
                     const VkMemoryPropertyFlags &properties,
                     VkImage &image,
                     VkDeviceMemory &imageMemory);

VkResult copyBufferToBuffer(VulkanCommandInfo vci, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

VkResult copyBufferToImage(VulkanCommandInfo vci, VkBuffer buffer, VkImage image, std::vector<VkBufferImageCopy> regions);

VkResult beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer &commandBuffer);
VkResult endSingleTimeCommands(VkDevice device,
                               VkCommandPool commandPool,
                               VkQueue queue,
                               VkCommandBuffer commandBuffer,
                               bool freeCommandBuffer = true,
                               uint64_t timeout = VULKAN_TIMEOUT_10S);

/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, creates and destroys a command buffer
*/
VkResult transitionImageLayout(VulkanCommandInfo vci,
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
VkResult transitionImageLayout(VulkanCommandInfo vci,
                               VkImage image,
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout,
                               VkImageSubresourceRange resourceRange);
/**
    Submit a command to transition the layout of an image from oldLayout to newLayout, using a custom resource range for the image
    uses an existing command buffer
*/
void transitionImageLayout(VkCommandBuffer cmdBuf,
                           VkImage image,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkImageSubresourceRange resourceRange);

VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat *depthFormat);

VkFormat autoChooseFormat(ColorSpace colorSpace, ColorDepth colorDepth, uint32_t channels);

VkDeviceOrHostAddressKHR getBufferDeviceAddress(VkDevice device, VkBuffer buffer);

}  // namespace vengine

#endif