#include "VulkanAccelerationStructure.hpp"

#include "VulkanMesh.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanDeviceFunctions.hpp"

namespace vengine
{

VulkanAccelerationStructure::VulkanAccelerationStructure()
{
}

VkResult VulkanAccelerationStructure::initializeBottomLevelAcceslerationStructure(VulkanCommandInfo vci,
                                                                                  const VulkanMesh &mesh,
                                                                                  const glm::mat4 &t)
{
    assert(!initialized());

    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

    const std::vector<Vertex> &vertices = mesh.vertices();
    const std::vector<uint32_t> indices = mesh.indices();
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    VkTransformMatrixKHR transformMatrix = {
        t[0][0], t[1][0], t[2][0], t[3][0], t[0][1], t[1][1], t[2][1], t[3][1], t[0][2], t[1][2], t[2][2], t[3][2]};

    /* Create buffers for Ray Tracing for mesh data and the transformation matrix */
    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VulkanBuffer transformBuffer;
    // TODO use transform buffer from the dybamic UBO that stores all the transform matrices?
    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       sizeof(transformMatrix),
                                       usageFlags,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &transformMatrix,
                                       transformBuffer));

    /* Get the device address of the geometry buffers and push them to the scene objects vector */
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vertexBufferDeviceAddress.deviceAddress = mesh.vertexBuffer().address().deviceAddress;
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    indexBufferDeviceAddress.deviceAddress = mesh.indexBuffer().address().deviceAddress;
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(vci.device, transformBuffer.buffer()).deviceAddress;

    std::vector<uint32_t> numTriangles = {indexCount / 3};
    uint32_t maxVertex = static_cast<uint32_t>(vertices.size());

    /* Specify an acceleration structure geometry for the mesh */
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    // accelerationStructureGeometry.flags =
    //     (material->transparent() ? VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR : VK_GEOMETRY_OPAQUE_BIT_KHR);
    accelerationStructureGeometry.flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
    accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
    accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;
    /* Specify all acceleration structure geometries */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    /* Get required size for the acceleration structure */
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    devF->vkGetAccelerationStructureBuildSizesKHR(vci.device,
                                                  VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                  &accelerationStructureBuildGeometryInfo,
                                                  numTriangles.data(),
                                                  &accelerationStructureBuildSizesInfo);

    /* Create bottom level acceleration structure buffer with the specified size */
    VULKAN_CHECK_CRITICAL(
        createBuffer(vci.physicalDevice,
                     vci.device,
                     accelerationStructureBuildSizesInfo.accelerationStructureSize,
                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_buffer));

    /* Create the bottom level accelleration structure */
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_buffer.buffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    devF->vkCreateAccelerationStructureKHR(vci.device, &accelerationStructureCreateInfo, nullptr, &m_handle);

    /* Create a scratch buffer used during the build of the bottom level acceleration structure */
    VulkanBuffer scratchBuffer;
    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       accelerationStructureBuildSizesInfo.buildScratchSize,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       scratchBuffer));

    /* Build accelleration structure */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = m_handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.address().deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles[0];
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
        &accelerationStructureBuildRangeInfo};

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    devF->vkCmdBuildAccelerationStructuresKHR(
        commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    /* Get the device address */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = m_handle;
    m_buffer.address().deviceAddress = devF->vkGetAccelerationStructureDeviceAddressKHR(vci.device, &accelerationDeviceAddressInfo);

    transformBuffer.destroy(vci.device);
    scratchBuffer.destroy(vci.device);

    m_initialzed = true;
    return VK_SUCCESS;
}

VkResult VulkanAccelerationStructure::initializeTopLevelAcceslerationStructure(
    VulkanCommandInfo vci,
    const std::vector<VkAccelerationStructureInstanceKHR> &instances)
{
    assert(!initialized());

    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

    uint32_t instancesCount = static_cast<uint32_t>(instances.size());

    /* Create buffer to copy the instances */
    VulkanBuffer instancesBuffer;
    VULKAN_CHECK_CRITICAL(
        createBuffer(vci.physicalDevice,
                     vci.device,
                     instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     instances.data(),
                     instancesBuffer));
    /* Get address of the instances buffer */
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(vci.device, instancesBuffer.buffer()).deviceAddress;

    /* Create an acceleration structure geometry to reference the instances buffer */
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;
    /* Get the size info for the acceleration structure */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    /* Get build size of accelleration structure */
    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    devF->vkGetAccelerationStructureBuildSizesKHR(vci.device,
                                                  VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                  &accelerationStructureBuildGeometryInfo,
                                                  &instancesCount,
                                                  &accelerationStructureBuildSizesInfo);

    /* Create acceleration structure buffer */
    VULKAN_CHECK_CRITICAL(
        createBuffer(vci.physicalDevice,
                     vci.device,
                     accelerationStructureBuildSizesInfo.accelerationStructureSize,
                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_buffer));

    /* Create acceleration structure */
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_buffer.buffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    devF->vkCreateAccelerationStructureKHR(vci.device, &accelerationStructureCreateInfo, nullptr, &m_handle);

    /* Create a scratch buffer used during build of the top level acceleration structure */
    VulkanBuffer scratchBuffer;
    VULKAN_CHECK_CRITICAL(createBuffer(vci.physicalDevice,
                                       vci.device,
                                       accelerationStructureBuildSizesInfo.buildScratchSize,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                       scratchBuffer));

    /* Build accelleration structure */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = m_handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.address().deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = instancesCount;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
        &accelerationStructureBuildRangeInfo};

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(vci.device, vci.commandPool, commandBuffer));

    devF->vkCmdBuildAccelerationStructuresKHR(
        commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(vci.device, vci.commandPool, vci.queue, commandBuffer));

    /* Get device address of accelleration structure */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = m_handle;
    m_buffer.address().deviceAddress = devF->vkGetAccelerationStructureDeviceAddressKHR(vci.device, &accelerationDeviceAddressInfo);

    instancesBuffer.destroy(vci.device);
    scratchBuffer.destroy(vci.device);

    m_initialzed = true;
    return VK_SUCCESS;
}

void VulkanAccelerationStructure::destroy(VkDevice device)
{
    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

    devF->vkDestroyAccelerationStructureKHR(device, m_handle, nullptr);
    m_buffer.destroy(device);

    m_initialzed = false;
}

}  // namespace vengine