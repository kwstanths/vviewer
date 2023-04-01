#include "VulkanRendererRayTracing.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <sys/types.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <utils/Console.hpp>
#include <utils/Timer.hpp>

#include "core/Lights.hpp"
#include "core/Materials.hpp"
#include "utils/ECS.hpp"
#include "utils/ImageUtils.hpp"
#include "vulkan/IncludeVulkan.hpp"
#include "vulkan/VulkanStructs.hpp"
#include "vulkan/VulkanUtils.hpp"
#include "vulkan/Shader.hpp"
#include "vulkan/VulkanLimits.hpp"

VulkanRendererRayTracing::VulkanRendererRayTracing(VulkanContext& vkctx, 
    VulkanMaterialSystem& materialSystem,
    VulkanTextures& textures) 
    : m_vkctx(vkctx), m_materialSystem(materialSystem), m_textures(textures)
{
}

void VulkanRendererRayTracing::initResources(VkFormat format)
{
    /* Check if physical device supports ray tracing */
    bool supportsRayTracing = VulkanRendererRayTracing::checkRayTracingSupport(m_vkctx.physicalDevice());
    if (!supportsRayTracing)
    {
        throw std::runtime_error("Ray tracing is not supported");
    }

    m_format = format;

    m_physicalDeviceProperties = m_vkctx.physicalDeviceProperties();

    m_device = m_vkctx.device();
    m_commandPool = m_vkctx.graphicsCommandPool();
    m_queue = m_vkctx.graphicsQueue();

    /* Get ray tracing specific device functions pointers */
    {
        m_devF.vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddressKHR"));
        m_devF.vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
        m_devF.vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkBuildAccelerationStructuresKHR"));
        m_devF.vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
        m_devF.vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
        m_devF.vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
        m_devF.vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        m_devF.vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));
        m_devF.vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        m_devF.vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
    }

    /* Get ray tracing pipeline properties */
    {
        m_rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 deviceProperties2{};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &m_rayTracingPipelineProperties;
        vkGetPhysicalDeviceProperties2(m_vkctx.physicalDevice(), &deviceProperties2);

        /* Get acceleration structure properties */
        m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &m_accelerationStructureFeatures;
        vkGetPhysicalDeviceFeatures2(m_vkctx.physicalDevice(), &deviceFeatures2);
    }

    createUniformBuffers();
    createRayTracingPipeline();
    createShaderBindingTable();
    createDescriptorSets();

    /* Default resolution */
    m_width = 1024;
    m_height = 1024;
    createStorageImage();

    m_renderResultOutputFileName = "test";
    m_renderResultOutputFileType = OutputFileType::HDR;

    m_isInitialized = true;
}

void VulkanRendererRayTracing::releaseRenderResources()
{
    for(auto& mb : m_meshBuffers)
    {
        mb.destroy(m_device);
    }
    m_meshBuffers.clear();

    destroyAccellerationStructures();
}

void VulkanRendererRayTracing::releaseResources()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    m_shaderRayGenBuffer.destroy(m_device);
    m_shaderRayMissBuffer.destroy(m_device);
    m_shaderRayCHitBuffer.destroy(m_device);

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    m_uniformBufferScene.destroy(m_device);
    m_uniformBufferObjectDescription.destroy(m_device);

    m_tempImage.destroy(m_device);
    m_renderResult.destroy(m_device);
}

bool VulkanRendererRayTracing::isInitialized() const
{
    return m_isInitialized;
}

void VulkanRendererRayTracing::renderScene(const VulkanScene* scene)
{
    if (m_renderInProgress) return;

    m_renderProgress = 0.0F;

    utils::Timer timer;
    timer.Start();

    std::vector<glm::mat4> sceneObjectMatrices;
    std::vector<std::shared_ptr<SceneObject>> sceneObjects = scene->getSceneObjects(sceneObjectMatrices);
    if (sceneObjects.size() == 0)
    {
        utils::ConsoleWarning("Trying to ray trace an empty scene");
        return;
    }

    m_renderInProgress = true;

    const SceneData& sceneData = scene->getSceneData();
    
    /* Create bottom level acceleration sturcutres out of all the meshes in the scene */
    for (size_t i = 0; i < sceneObjects.size(); i++)
    {
        auto so = sceneObjects[i];
        if (!so->has<ComponentMesh>() || !so->has<ComponentMaterial>()) continue;
        
        auto transform = sceneObjectMatrices[i];
        auto mesh = std::static_pointer_cast<VulkanMesh>(so->get<ComponentMesh>().mesh);
        auto material = so->get<ComponentMaterial>().material;

        AccelerationStructure blas = createBottomLevelAccelerationStructure(*mesh, glm::mat4(1.0f), material->getMaterialIndex());
        m_blas.push_back(std::make_pair(blas, transform));
    }
    
    /* Create a top level accelleration structure out of all the blas */
    m_tlas = createTopLevelAccelerationStructure();
    
    /* Prepare scene lights */
    auto sceneLights = prepareSceneLights(scene, sceneObjects);
    m_rayTracingData.lights.r = sceneLights.size();
    updateUniformBuffers(sceneData, m_rayTracingData, sceneLights);
    
    /* Will use buffer index 0 for this renderer */
    m_materialSystem.updateBuffers(0);

    render();
    
    releaseRenderResources();

    timer.Stop();

    std::string fileExtension = (getRenderOutputFileType() == OutputFileType::PNG) ? "png" : "hdr";
    std::string filename = getRenderOutputFileName() + "." + fileExtension;
    utils::ConsoleInfo("Scene rendered: " + filename + " in: " + std::to_string(timer.ToInt()) + "ms");

    m_renderInProgress = false;
}

void VulkanRendererRayTracing::setRenderResolution(uint32_t width, uint32_t height)
{
    m_renderResult.destroy(m_device);
    m_tempImage.destroy(m_device);

    m_width = width;
    m_height = height;
    createStorageImage(); 
}

void VulkanRendererRayTracing::getRenderResolution(uint32_t& width, uint32_t& height) const
{
    width = m_width;
    height = m_height;
}

std::vector<const char *> VulkanRendererRayTracing::getRequiredExtensions()
{
    return { "VK_KHR_acceleration_structure", 
        "VK_KHR_ray_tracing_pipeline", 
        "VK_KHR_buffer_device_address", 
        "VK_KHR_deferred_host_operations", 
        "VK_EXT_descriptor_indexing", 
        "VK_KHR_spirv_1_4", 
        "VK_KHR_shader_float_controls", 
        "VK_KHR_maintenance3"};
}

bool VulkanRendererRayTracing::checkRayTracingSupport(VkPhysicalDevice device)
{
    /* Get required extensions */
    std::vector<const char *> requiredExtensions = VulkanRendererRayTracing::getRequiredExtensions();

    /* Get the number of extensions supported by the device */
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    /* Check if the required extensions are supported */
    auto availableExtensions = std::vector<VkExtensionProperties>(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.data()); //populate buffer
    for (auto requiredExetension : requiredExtensions)
    {
        bool found = false;
        for (auto& availableExtension : availableExtensions) {
            if (strcmp(requiredExetension, availableExtension.extensionName) == 0)
            {
                found = true;
                continue;
            }
        }
        if (!found)
        {
            return false;
        }
    }

    return true;
}

uint64_t VulkanRendererRayTracing::getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return m_devF.vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}

VulkanRendererRayTracing::AccelerationStructure VulkanRendererRayTracing::createBottomLevelAccelerationStructure(const VulkanMesh& mesh, const glm::mat4& t, const uint32_t materialIndex)
{
    const std::vector<Vertex>& vertices = mesh.getVertices();
    const std::vector<uint32_t> indices = mesh.getIndices();
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    VkTransformMatrixKHR transformMatrix = {
        t[0][0], t[1][0], t[2][0], t[3][0],
        t[0][1], t[1][1], t[2][1], t[3][1],
        t[0][2], t[1][2], t[2][2], t[3][2]
    };

    /* Create buffers for RT for mesh data and the transformation matrix */
    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VkBuffer vertexBuffer = mesh.getVertexBuffer().vkbuffer();
    VkDeviceMemory vertexBufferMemory = mesh.getVertexBuffer().vkmemory();

    VkBuffer indexBuffer = mesh.getIndexBuffer().vkbuffer();
    VkDeviceMemory indexBufferMemory = mesh.getIndexBuffer().vkmemory();

    // TODO usage transform buffer from the dybamic UBO that stores the mvp matrices?
    VulkanBuffer transformBuffer;
    createBuffer(m_vkctx.physicalDevice(),
        m_device,
        sizeof(transformMatrix),
        usageFlags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &transformMatrix,
        transformBuffer);

    /* Get the device address of the geometry  buffers and push them to the scene objects vector */
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, vertexBuffer);
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, indexBuffer);
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, transformBuffer.vkbuffer());
    m_sceneObjects.push_back({ vertexBufferDeviceAddress.deviceAddress, indexBufferDeviceAddress.deviceAddress, materialIndex });

    std::vector<uint32_t> numTriangles = { indexCount / 3 };
    uint32_t maxVertex = static_cast<uint32_t>(vertices.size());

    /* Specify an acceleration structure geometry for the mesh */
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
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
    m_devF.vkGetAccelerationStructureBuildSizesKHR(
        m_device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        numTriangles.data(),
        &accelerationStructureBuildSizesInfo);
    /* Create bottom level acceleration structure buffer with the specified size */
    AccelerationStructure blas;
    createAccelerationStructureBuffer(m_vkctx.physicalDevice(),
        m_device,
        accelerationStructureBuildSizesInfo,
        blas.handle,
        blas.deviceAddress,
        blas.memory,
        blas.buffer);
    /* Create the bottom level accelleration structure */
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = blas.buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    m_devF.vkCreateAccelerationStructureKHR(m_device, &accelerationStructureCreateInfo, nullptr, &blas.handle);

    /* Create a scratch buffer used during the build of the bottom level acceleration structure */
    RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    /* Build accelleration structure */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = blas.handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = numTriangles[0];
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_device, m_commandPool);
    
    m_devF.vkCmdBuildAccelerationStructuresKHR(
        commandBuffer,
        1,
        &accelerationBuildGeometryInfo,
        accelerationBuildStructureRangeInfos.data());

    endSingleTimeCommands(m_device, m_commandPool, m_queue, commandBuffer);

    /* Get the device address */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = blas.handle;
    blas.deviceAddress = m_devF.vkGetAccelerationStructureDeviceAddressKHR(m_device, &accelerationDeviceAddressInfo);

    m_meshBuffers.push_back(transformBuffer);
    deleteScratchBuffer(scratchBuffer);

    return blas;
}

VulkanRendererRayTracing::AccelerationStructure VulkanRendererRayTracing::createTopLevelAccelerationStructure()
{
    /* Create a buffer to hold top level instances */
    std::vector<VkAccelerationStructureInstanceKHR> instances(m_blas.size());
    for (size_t i = 0; i < m_blas.size(); i++)
    {
        auto& t = m_blas[i].second;
        /* Set transform matrix per blas */
        VkTransformMatrixKHR transformMatrix = {
            t[0][0], t[1][0], t[2][0], t[3][0],
            t[0][1], t[1][1], t[2][1], t[3][1],
            t[0][2], t[1][2], t[2][2], t[3][2] 
        };

        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = static_cast<uint32_t>(i);    /* The custom index specifies where the reference of this object 
                                                                        is inside the array of references that is passed in the shader. 
                                                                        More specifically, this reference will tell the shader where the 
                                                                        geometry buffers for this object are, the meshe's index inside 
                                                                        the ObjectDescription buffer */
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = 0;    /* TODO Specify here the chit shader to be used from the SBT for this instance */
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = m_blas[i].first.deviceAddress;
    }
    uint32_t instancesCount = static_cast<uint32_t>(instances.size());

    /* Create buffer to copy the instances */
    VulkanBuffer instancesBuffer;
    createBuffer(m_vkctx.physicalDevice(),
        m_device,
        instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        instances.data(),
        instancesBuffer);
    /* Get address of the instances buffer */
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, instancesBuffer.vkbuffer());

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
    m_devF.vkGetAccelerationStructureBuildSizesKHR(
        m_device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &instancesCount,
        &accelerationStructureBuildSizesInfo);

    /* Create acceleration structure buffer */
    AccelerationStructure tlas;
    createAccelerationStructureBuffer(m_vkctx.physicalDevice(),
        m_device,
        accelerationStructureBuildSizesInfo,
        tlas.handle,
        tlas.deviceAddress,
        tlas.memory,
        tlas.buffer);
    /* Create acceleration structure */
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = tlas.buffer;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    m_devF.vkCreateAccelerationStructureKHR(m_device, &accelerationStructureCreateInfo, nullptr, &tlas.handle);

    /* Create a scratch buffer used during build of the top level acceleration structure */
    RayTracingScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

    /* Build accelleration structure */
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = tlas.handle;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
    accelerationStructureBuildRangeInfo.primitiveCount = instancesCount;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(m_device, m_commandPool);

    m_devF.vkCmdBuildAccelerationStructuresKHR(
        commandBuffer,
        1,
        &accelerationBuildGeometryInfo,
        accelerationBuildStructureRangeInfos.data());

    endSingleTimeCommands(m_device, m_commandPool, m_queue, commandBuffer);

    /* Get device address of accelleration structure */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = tlas.handle;
    tlas.deviceAddress = m_devF.vkGetAccelerationStructureDeviceAddressKHR(m_device, &accelerationDeviceAddressInfo);

    deleteScratchBuffer(scratchBuffer);
    m_meshBuffers.push_back(instancesBuffer);

    return tlas;
}

void VulkanRendererRayTracing::destroyAccellerationStructures()
{
    m_sceneObjects.clear();

    for (auto& blas : m_blas) {
        m_devF.vkDestroyAccelerationStructureKHR(m_device, blas.first.handle, nullptr);
        vkDestroyBuffer(m_device, blas.first.buffer, nullptr);
        vkFreeMemory(m_device, blas.first.memory, nullptr);
    }
    m_blas.clear();

    m_devF.vkDestroyAccelerationStructureKHR(m_device, m_tlas.handle, nullptr);
    vkDestroyBuffer(m_device, m_tlas.buffer, nullptr);
    vkFreeMemory(m_device, m_tlas.memory, nullptr);
}

void VulkanRendererRayTracing::createStorageImage()
{
    /* Create render target used during ray tracing */
    bool ret = createImage(m_vkctx.physicalDevice(),
        m_device,
        m_width,
        m_height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        m_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_renderResult.image,
        m_renderResult.memory);

    m_renderResult.view = createImageView(m_device,
        m_renderResult.image,
        m_format,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1);

    /* Create temp image used to copy render result from gpu memory to cpu memory */
    ret = createImage(m_vkctx.physicalDevice(),
        m_device,
        m_width,
        m_height,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        m_format,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_tempImage.image,
        m_tempImage.memory);

    /* Transition images to the approriate layout ready for render */
    VkCommandBuffer cmdBuf = beginSingleTimeCommands(m_device, m_commandPool);
    
    transitionImageLayout(cmdBuf,
        m_renderResult.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL, 
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    transitionImageLayout(cmdBuf,
        m_tempImage.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
 
    endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf);
}

void VulkanRendererRayTracing::createUniformBuffers()
{
    /* The scene buffer holds [SceneData | RayTracingData | VULKAN_LIMITS_MAX_LIGHTS * LightRT ] */
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t totalSceneBufferSize = alignedSize(sizeof(SceneData), alignment) + 
                                    alignedSize(sizeof(RayTracingData), alignment) + 
                                    alignedSize(VULKAN_LIMITS_MAX_LIGHTS * sizeof(LightRT), alignment);

    /* Create a buffer to hold the scene buffer */
    createBuffer(m_vkctx.physicalDevice(),
        m_device,
        totalSceneBufferSize,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBufferScene);

    /* Create a buffer to hold the object descriptions data */
    createBuffer(m_vkctx.physicalDevice(),
        m_device,
        VULKAN_LIMITS_MAX_OBJECTS * sizeof(ObjectDescriptionRT),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBufferObjectDescription);
}

void VulkanRendererRayTracing::updateUniformBuffers(const SceneData& sceneData, const RayTracingData& rtData, const std::vector<LightRT>& lights)
{
    if (m_sceneObjects.size() > VULKAN_LIMITS_MAX_OBJECTS)
    {
        throw std::runtime_error("VulkanRendererRayTracing::updateUniformBuffers(): Number of objects exceeded");
    }
    if (lights.size() > VULKAN_LIMITS_MAX_LIGHTS)
    {
        throw std::runtime_error("VulkanRendererRayTracing::updateUniformBuffers(): Number of lights exceeded");
    }

    /* Update scene buffer */
    {
        VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
        
        void* data;
        vkMapMemory(m_device, m_uniformBufferScene.vkmemory(), 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));    /* Copy scene data struct */
        memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment), &rtData, sizeof(RayTracingData));   /* Copy ray tracing data struct */
        memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment) + alignedSize(sizeof(RayTracingData), alignment), lights.data(), lights.size() * sizeof(LightRT)); /* Copy lights vector */
        vkUnmapMemory(m_device, m_uniformBufferScene.vkmemory());
    }

    /* Update description of geometry objects */
    {
        void* data;
        vkMapMemory(m_device, m_uniformBufferObjectDescription.vkmemory(), 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, m_sceneObjects.data(), m_sceneObjects.size() * sizeof(ObjectDescriptionRT));
        vkUnmapMemory(m_device, m_uniformBufferObjectDescription.vkmemory());
    }
}

void VulkanRendererRayTracing::updateUniformBuffersRayTracingData(const RayTracingData& rtData)
{
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

    void* data;
    vkMapMemory(m_device, m_uniformBufferScene.vkmemory(), 0, VK_WHOLE_SIZE, 0, &data);
    memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment), &rtData, sizeof(RayTracingData));   /* Copy ray tracing data struct */
    
    vkUnmapMemory(m_device, m_uniformBufferScene.vkmemory());
}

void VulkanRendererRayTracing::createDescriptorSets()
{
    /* First set is:
    * 1 accelleration structure
    * 1 storage image for output
    * 1 uniform buffer for scene data
    * 1 storage buffer for references to scene object geometry
    * 1 uniform buffer for light data
    */
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes = poolSizes.data();
    descriptorPoolInfo.maxSets = 1;
    VkResult res = vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createDescriptorSets(): Failed to create a descriptor pool: " + std::to_string(res));
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;
    descriptorSetAllocateInfo.pSetLayouts = &m_descriptorSetLayout;
    descriptorSetAllocateInfo.descriptorSetCount = 1;
    res = vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createDescriptorSets(): Failed to create a descriptor set: " + std::to_string(res));
    }
}

void VulkanRendererRayTracing::updateDescriptorSets()
{
    VkDeviceSize uniformBufferAlignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

    /* Update accelleration structure binding */
    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &m_tlas.handle;
    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    /* The specialized acceleration structure descriptor has to be chained */
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = m_descriptorSet;
    accelerationStructureWrite.dstBinding = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    /* Update storage image binding */
    VkDescriptorImageInfo storageImageDescriptor{};
    storageImageDescriptor.imageView = m_renderResult.view;
    storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    VkWriteDescriptorSet resultImageWrite{};
    resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    resultImageWrite.dstSet = m_descriptorSet;
    resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    resultImageWrite.dstBinding = 1;
    resultImageWrite.pImageInfo = &storageImageDescriptor;
    resultImageWrite.descriptorCount = 1;

    /* Update scene data binding */
    VkDescriptorBufferInfo sceneDataDescriptor{};
    sceneDataDescriptor.buffer = m_uniformBufferScene.vkbuffer();
    sceneDataDescriptor.offset = 0;
    sceneDataDescriptor.range = sizeof(SceneData);
    VkWriteDescriptorSet sceneDataWrite{};
    sceneDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sceneDataWrite.dstSet = m_descriptorSet;
    sceneDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sceneDataWrite.dstBinding = 2;
    sceneDataWrite.pBufferInfo = &sceneDataDescriptor;
    sceneDataWrite.descriptorCount = 1;

    /* Update ray tracing data binding, uses the same buffer with the scene data */
    VkDescriptorBufferInfo rayTracingDataDescriptor{};
    rayTracingDataDescriptor.buffer = m_uniformBufferScene.vkbuffer();
    rayTracingDataDescriptor.offset = alignedSize(sizeof(SceneData), uniformBufferAlignment);
    rayTracingDataDescriptor.range = sizeof(RayTracingData);
    VkWriteDescriptorSet rayTracingDataWrite{};
    rayTracingDataWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    rayTracingDataWrite.dstSet = m_descriptorSet;
    rayTracingDataWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    rayTracingDataWrite.dstBinding = 3;
    rayTracingDataWrite.pBufferInfo = &rayTracingDataDescriptor;
    rayTracingDataWrite.descriptorCount = 1;

    /* Update object description binding */
    VkDescriptorBufferInfo objectDescriptionDataDesriptor{};
    objectDescriptionDataDesriptor.buffer = m_uniformBufferObjectDescription.vkbuffer();
    objectDescriptionDataDesriptor.offset = 0;
    objectDescriptionDataDesriptor.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet objectDescriptionWrite{};
    objectDescriptionWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    objectDescriptionWrite.dstSet = m_descriptorSet;
    objectDescriptionWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectDescriptionWrite.dstBinding = 4;
    objectDescriptionWrite.pBufferInfo = &objectDescriptionDataDesriptor;
    objectDescriptionWrite.descriptorCount = 1;

    /* Update lights buffer binding */
    VkDescriptorBufferInfo lightsDescriptor{};
    lightsDescriptor.buffer = m_uniformBufferScene.vkbuffer();
    lightsDescriptor.offset = alignedSize(sizeof(SceneData), uniformBufferAlignment) + alignedSize(sizeof(RayTracingData), uniformBufferAlignment);
    lightsDescriptor.range = alignedSize(VULKAN_LIMITS_MAX_LIGHTS * sizeof(LightRT), uniformBufferAlignment);
    VkWriteDescriptorSet lightsWrite{};
    lightsWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    lightsWrite.dstSet = m_descriptorSet;
    lightsWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightsWrite.dstBinding = 5;
    lightsWrite.pBufferInfo = &lightsDescriptor;
    lightsWrite.descriptorCount = 1;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        accelerationStructureWrite,
        resultImageWrite,
        sceneDataWrite,
        rayTracingDataWrite,
        objectDescriptionWrite,
        lightsWrite
    };
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void VulkanRendererRayTracing::createRayTracingPipeline()
{
    /* Create the main ray tracing descriptor set */
    {
        /* binding 0, the accelleration strucure */
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
        accelerationStructureLayoutBinding.binding = 0;
        accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureLayoutBinding.descriptorCount = 1;
        accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        /* Binding 1, the output image */
        VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
        resultImageLayoutBinding.binding = 1;
        resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageLayoutBinding.descriptorCount = 1;
        resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        /* Binding 2, the scene data */
        VkDescriptorSetLayoutBinding sceneDataBufferBinding{};
        sceneDataBufferBinding.binding = 2;
        sceneDataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataBufferBinding.descriptorCount = 1;
        sceneDataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

        /* Binding 3, the ray tracing data */
        VkDescriptorSetLayoutBinding rayTracingDataBufferBinding{};
        rayTracingDataBufferBinding.binding = 3;
        rayTracingDataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        rayTracingDataBufferBinding.descriptorCount = 1;
        rayTracingDataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        /* Binding 4, the object descriptions buffer */
        VkDescriptorSetLayoutBinding objectDescrptionBufferBinding{};
        objectDescrptionBufferBinding.binding = 4;
        objectDescrptionBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        objectDescrptionBufferBinding.descriptorCount = 1;
        objectDescrptionBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        /* Binding 5, the lights buffer */
        VkDescriptorSetLayoutBinding lightsBufferBinding{};
        lightsBufferBinding.binding = 5;
        lightsBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightsBufferBinding.descriptorCount = 1;
        lightsBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        std::vector<VkDescriptorSetLayoutBinding> bindings({
            accelerationStructureLayoutBinding,
            resultImageLayoutBinding,
            sceneDataBufferBinding,
            rayTracingDataBufferBinding,
            objectDescrptionBufferBinding,
            lightsBufferBinding
        });

        /* 1 set only */
        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo{};
        descriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetlayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSetlayoutInfo.pBindings = bindings.data();
        VkResult res = vkCreateDescriptorSetLayout(m_device, &descriptorSetlayoutInfo, nullptr, &m_descriptorSetLayout);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error("VulkanRayTracingRenderer::createRayTracingPipeline(): Failed to create a descriptor set layout: " + std::to_string(res));
        }

    }

    /* Create pipeline layout */
    {
        std::array<VkDescriptorSetLayout, 3> setsLayouts = { m_descriptorSetLayout, m_materialSystem.layoutMaterial(), m_textures.layoutTextures() };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setsLayouts.size());
        pipelineLayoutInfo.pSetLayouts = setsLayouts.data();
        VkResult res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (res != VK_SUCCESS)
        {
            throw std::runtime_error("VulkanRayTracingRenderer::createRayTracingPipeline(): Failed to create a pipeline layout: " + std::to_string(res));
        }
    }

    /*
        Setup ray tracing shader groups
    */
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    /* Ray generation group */
    {
        VkPipelineShaderStageCreateInfo rayGenShaderStageInfo{};
        rayGenShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rayGenShaderStageInfo.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        rayGenShaderStageInfo.module = Shader::load(m_device, readSPIRV("shaders/SPIRV/rt/raygen.rgen.spv"));
        rayGenShaderStageInfo.pName = "main";
        shaderStages.push_back(rayGenShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Miss groups: one for primary ray miss, one for shadow raw miss */
    {
        VkPipelineShaderStageCreateInfo rayMissShaderStageInfo{};
        rayMissShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rayMissShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        rayMissShaderStageInfo.module = Shader::load(m_device, readSPIRV("shaders/SPIRV/rt/raymiss.rmiss.spv"));
        rayMissShaderStageInfo.pName = "main";
        shaderStages.push_back(rayMissShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        m_shaderGroups.push_back(shaderGroup);

        VkPipelineShaderStageCreateInfo shadowMissShaderStageInfo{};
        shadowMissShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shadowMissShaderStageInfo.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        shadowMissShaderStageInfo.module = Shader::load(m_device, readSPIRV("shaders/SPIRV/rt/shadow.rmiss.spv"));
        shadowMissShaderStageInfo.pName = "main";
        shaderStages.push_back(shadowMissShaderStageInfo);
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Closest hit group */
    {
        VkPipelineShaderStageCreateInfo rayClosestHitShaderStageInfo{};
        rayClosestHitShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        rayClosestHitShaderStageInfo.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        rayClosestHitShaderStageInfo.module = Shader::load(m_device, readSPIRV("shaders/SPIRV/rt/raychit.rchit.spv"));
        rayClosestHitShaderStageInfo.pName = "main";
        shaderStages.push_back(rayClosestHitShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
        shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;  /* The closet hit shader to use for this hit group */
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Spec only guarantees 1 level of "recursion". Check for that sad possibility here */
    if (m_rayTracingPipelineProperties.maxRayRecursionDepth <= 1) {
        throw std::runtime_error("VulkanRayTracingRenderer::createRayTracingPipeline(): Device doesn't support ray recursion");
    }

    /*
        Create the ray tracing pipeline
    */
    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
    rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayTracingPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    rayTracingPipelineInfo.pStages = shaderStages.data();
    rayTracingPipelineInfo.groupCount = static_cast<uint32_t>(m_shaderGroups.size());
    rayTracingPipelineInfo.pGroups = m_shaderGroups.data();
    rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 2;
    rayTracingPipelineInfo.layout = m_pipelineLayout;
    VkResult res = m_devF.vkCreateRayTracingPipelinesKHR(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_pipeline);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createRayTracingPipeline(): Failed to create a ray tracing pipeline: " + std::to_string(res));
    }

    /* Destroy shader modules */
    for (auto& s : shaderStages) {
        vkDestroyShaderModule(m_device, s.module, nullptr);
    }
}

void VulkanRendererRayTracing::createShaderBindingTable()
{
    const uint32_t handleSize = m_rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = alignedSize(m_rayTracingPipelineProperties.shaderGroupHandleSize, m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(m_shaderGroups.size());
    const uint32_t sbtSize = groupCount * handleSizeAligned;    /* Size in bytes */

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    VkResult res = m_devF.vkGetRayTracingShaderGroupHandlesKHR(m_device, m_pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createShaderBindingTable(): Failed to get the shader group handles: " + std::to_string(res));
    }

    /* Create shader binding tables for each group, ray gen, miss and chit, and store the shader group handles */
    const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    createBuffer(m_vkctx.physicalDevice(), m_device,     handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayGenBuffer);
    createBuffer(m_vkctx.physicalDevice(), m_device, 2 * handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayMissBuffer);
    createBuffer(m_vkctx.physicalDevice(), m_device,     handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayCHitBuffer);

    /* 1 ray gen group */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayGenBuffer.vkmemory(), 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data(), handleSize);
        vkUnmapMemory(m_device, m_shaderRayGenBuffer.vkmemory());
    }
    /* 2 ray miss groups */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayMissBuffer.vkmemory(), 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
        vkUnmapMemory(m_device, m_shaderRayMissBuffer.vkmemory());
    }
    /* 1 ray chit group */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayCHitBuffer.vkmemory(), 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
        vkUnmapMemory(m_device, m_shaderRayCHitBuffer.vkmemory());
    }
}

void VulkanRendererRayTracing::render()
{
    /* Get strided device addresses for the shader binding tables where the shader group handles are stored. TODO get this out of the render function */
    const uint32_t handleSizeAligned = alignedSize(m_rayTracingPipelineProperties.shaderGroupHandleSize, m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayGenBuffer.vkbuffer());
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;
    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayMissBuffer.vkbuffer());
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = 2 * handleSizeAligned;
    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayCHitBuffer.vkbuffer());
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;
    /* No callable shader binding tables */
    VkStridedDeviceAddressRegionKHR emptySbtEntry{};

    VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    
    uint32_t batchSize = 16U;
    uint32_t batches = getSamples() / batchSize;

    VkCommandBuffer commandBuffer;
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;
        
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    utils::ConsoleInfo("Launching render with: " + std::to_string(batches * batchSize) + " samples");
    for(uint32_t batch = 0; batch < batches; batch++)
    {
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        RayTracingData batchData = m_rayTracingData;
        batchData.samplesBatchesDepthIndex.r = batchSize;
        batchData.samplesBatchesDepthIndex.g = batches;
        batchData.samplesBatchesDepthIndex.a = batch;
        updateUniformBuffersRayTracingData(batchData);
        updateDescriptorSets();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);

        std::array<VkDescriptorSet, 3> descriptorSets = { m_descriptorSet, m_materialSystem.descriptor(0), m_textures.descriptorTextures() };
        vkCmdBindDescriptorSets(commandBuffer, 
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
            m_pipelineLayout, 
            0, 
            static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 
            0, nullptr
        );
        
        /*
            Dispatch the ray tracing commands
        */
        m_devF.vkCmdTraceRaysKHR(
            commandBuffer,
            &raygenShaderSbtEntry,
            &missShaderSbtEntry,
            &hitShaderSbtEntry,
            &emptySbtEntry,
            m_width,
            m_height,
            1);

        vkEndCommandBuffer(commandBuffer);

        /* Create fence to wait for the command to finish */
        VkFenceCreateInfo fenceCreateInfo{};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = 0;
        VkFence fence;
        vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence);

        /* Submit command buffer */
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        vkQueueSubmit(m_queue, 1, &submitInfo, fence);

        /* Wait for the fence to signal that command buffer has finished executing */
        VkResult res = vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_1S);
        if (res == VK_SUCCESS)
        {
            vkDestroyFence(m_device, fence, nullptr);
        } else {
            utils::ConsoleCritical("VulkanRendererRayTracing::render(): Render failed: " + std::to_string(res));
        }
    
        m_renderProgress = static_cast<float>(batch) / static_cast<float>(batches);
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);

    VkCommandBuffer cmdBuf = beginSingleTimeCommands(m_device, m_commandPool);

    /* Transition output render result to transfer source optimal */
    transitionImageLayout(cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
    
    /* Copy render result to temp image */
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.srcOffset = { 0, 0, 0 };
    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copyRegion.dstOffset = { 0, 0, 0 };
    copyRegion.extent = { m_width, m_height, 1 };
    vkCmdCopyImage(cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_tempImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    /* Transition render result back to layout general */
    transitionImageLayout(cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);

    VkResult res = endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf, true, VULKAN_TIMEOUT_100S);
    if (res != VK_SUCCESS)
    {
        utils::ConsoleCritical("VulkanRendererRayTracing::render(): Storing failed: " + std::to_string(res));
    } else {
        storeToDisk(m_renderResultOutputFileName, m_renderResultOutputFileType);
    }

    vkQueueWaitIdle(m_queue);
}

void VulkanRendererRayTracing::storeToDisk(std::string filename, OutputFileType type) const
{
    /* Store result from temp image to the disk */
    if (m_format != VK_FORMAT_R32G32B32A32_SFLOAT) {
        utils::ConsoleCritical("VulkanRayTracing::StoreToDisk(): Format supported is VK_FORMAT_R32G32B32A32_SFLOAT");
        return;
    }

    float* input;
    VkResult res = vkMapMemory(
        m_device,
        m_tempImage.memory,
        0,
        VK_WHOLE_SIZE,
        0,
        reinterpret_cast<void**>(&input));

    VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_device, m_tempImage.image, &subResource, &subResourceLayout);
    uint32_t rowPitchFloats = subResourceLayout.rowPitch / sizeof(float);

    std::vector<float>imageDataFloat(m_width * m_height * 4);
    uint32_t indexGPUIMage = subResourceLayout.offset;
    uint32_t indexOutputImage = 0;
    for (uint32_t i = 0; i < m_height; i++) {
        memcpy(&imageDataFloat[0] + indexOutputImage, &input[0] + indexGPUIMage, 4 * m_width * sizeof(float));
        indexGPUIMage += rowPitchFloats;
        indexOutputImage += 4 * m_width;
    }
    vkUnmapMemory(m_device, m_tempImage.memory);

    switch (type)
    {
    case OutputFileType::PNG:
    {
        /* TODO apply exposure */
        std::vector<unsigned char> imageDataInt(m_width * m_height * 4, 255);
        for (size_t i = 0; i < m_width * m_height * 4; i++)
        {
            imageDataInt[i] = linearToSRGB(imageDataFloat[i]);
        }
        stbi_write_png((filename + ".png").c_str(), m_width, m_height, 4, imageDataInt.data(), m_width * 4 * sizeof(char));
        break;
    }
    case OutputFileType::HDR:
    {
        stbi_write_hdr((filename + ".hdr").c_str(), m_width, m_height, 4, imageDataFloat.data());
        break;
    }
    default:
        utils::ConsoleWarning("VulkanRayTracing::StoreToDisk(): Unknown output file type");
        break;
    }

}

VulkanRendererRayTracing::RayTracingScratchBuffer VulkanRendererRayTracing::createScratchBuffer(VkDeviceSize size)
{
    RayTracingScratchBuffer scratchBuffer{};

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VkResult res = vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &scratchBuffer.handle);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createScratchBuffer(): Failed to create a scratch buffer: " + std::to_string(res));
    }

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(m_device, scratchBuffer.handle, &memoryRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_vkctx.physicalDevice(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    res = vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &scratchBuffer.memory);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createScratchBuffer(): Failed to allocate memory for a scratch buffer: " + std::to_string(res));
    }

    res = vkBindBufferMemory(m_device, scratchBuffer.handle, scratchBuffer.memory, 0);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createScratchBuffer(): Failed to bind memory for a scratch buffer: " + std::to_string(res));
    }

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.handle;
    scratchBuffer.deviceAddress = m_devF.vkGetBufferDeviceAddressKHR(m_device, &bufferDeviceAddressInfo);

    return scratchBuffer;
}

void VulkanRendererRayTracing::deleteScratchBuffer(RayTracingScratchBuffer& scratchBuffer)
{
    if (scratchBuffer.memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, scratchBuffer.memory, nullptr);
    }
    if (scratchBuffer.handle != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, scratchBuffer.handle, nullptr);
    }
}

std::vector<LightRT> VulkanRendererRayTracing::prepareSceneLights(const VulkanScene * scene, const std::vector<std::shared_ptr<SceneObject>>& sceneObjects)
{
    std::vector<LightRT> sceneLights;

    const SceneData& sceneData = scene->getSceneData();

    /* Environment map */
    if (scene->getAmbientIBL() > 0.0001F && scene->getSkybox() != nullptr)
    {
        sceneLights.push_back(LightRT());
        sceneLights.back().position.a = 3.F;
    }

    /* Directional light */
    if (sceneData.m_directionalLightColor.r > 0.0001F || sceneData.m_directionalLightColor.g > 0.0001F || sceneData.m_directionalLightColor.b > 0.0001F)
    {
        sceneLights.push_back(LightRT());
        sceneLights.back().position.a = 1.F;
        sceneLights.back().direction = sceneData.m_directionalLightDir;
        sceneLights.back().color = sceneData.m_directionalLightColor;
    }

    for(auto so : sceneObjects)
    {
        if (so->has<ComponentPointLight>())
        {
            auto pointLight = so->get<ComponentPointLight>().light;
            sceneLights.push_back(LightRT());
            sceneLights.back().position = glm::vec4(so->getWorldPosition(), 0.F);
            sceneLights.back().color = glm::vec4(pointLight->lightMaterial->color * pointLight->lightMaterial->intensity, 0.F);
        }

        if (so->has<ComponentMaterial>()) 
        {
            auto material = so->get<ComponentMaterial>().material;
            switch (material->getType())
            {
            case MaterialType::MATERIAL_PBR_STANDARD:
            {
                auto pbrStandard = std::static_pointer_cast<VulkanMaterialPBRStandard>(material);
                if (pbrStandard->getEmissive() > 0.0001F) {
                    // TODO add to scene lights 
                }
                break;
            }
            case MaterialType::MATERIAL_LAMBERT:
            {
                auto lambert = std::static_pointer_cast<VulkanMaterialPBRStandard>(material);
                if (lambert->getEmissive() > 0.0001F) {
                    // TODO add to scene lights
                }
                break;
            }
            default:
                throw std::runtime_error("VulkanRayTracing::prepareSceneLights(): Unexpected material");
            }
        }

    }

    return sceneLights;
}

