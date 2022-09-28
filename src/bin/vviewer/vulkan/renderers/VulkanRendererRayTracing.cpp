#include "VulkanRendererRayTracing.hpp"

#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <utils/Console.hpp>

#include "vulkan/Shader.hpp"

VulkanRendererRayTracing::VulkanRendererRayTracing()
{
}

void VulkanRendererRayTracing::initResources(VkPhysicalDevice physicalDevice, VkFormat format)
{
    /* Check if physical device supports ray tracing */
    std::vector< VkExtensionProperties> extensions;
    bool supportsRayTracing = VulkanRendererRayTracing::checkRayTracingSupport(physicalDevice, extensions);
    if (!supportsRayTracing)
    {
        throw std::runtime_error("Ray tracing is not supported");
    }

    m_physicalDevice = physicalDevice;
    m_format = format;

    /* Create logical device from scratch for ray tracing, since current version of qt(6.2.4) doesn't support setting custom features for the logical device 
    * thus the required ray tracing features cannot be enabled
    */
    m_device = createLogicalDevice(m_physicalDevice, m_queueFamilyIndex);
    vkGetDeviceQueue(m_device, m_queueFamilyIndex, 0, &m_queue);

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
        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);

        /* Get acceleration structure properties */
        m_accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &m_accelerationStructureFeatures;
        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
    }

    /* Create a command pool */
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.pNext = NULL;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolCreateInfo.queueFamilyIndex = m_queueFamilyIndex;

        VkResult res = vkCreateCommandPool(m_device, &commandPoolCreateInfo, NULL, &m_commandPool);
        if (res != VK_SUCCESS) {
            throw std::runtime_error("VulkanRayTracingRenderer::initResources(): Unable to create a command pool ");
        }
    }

    createUniformBuffers();
    createRayTracingPipeline();
    createShaderBindingTable();
    createDescriptorSets();
    m_isInitialized = true;
}

void VulkanRendererRayTracing::initSwapChainResources(VkExtent2D swapchainExtent)
{
    m_width = swapchainExtent.width;
    m_height = swapchainExtent.height;

    createStorageImage();
}

void VulkanRendererRayTracing::releaseSwapChainResources()
{
    m_renderResult.destroy(m_device);
}

void VulkanRendererRayTracing::releaseResources()
{
    /* TODO destroy descriptor set */
    /* TODO delete pipeline and rt pipeline */
    /* TODO destroy shader binding table */
    /* TODO delete command pool */
    /* TODO delete device */
}

bool VulkanRendererRayTracing::isInitialized() const
{
    return m_isInitialized;
}

void VulkanRendererRayTracing::renderScene(const VulkanScene* scene)
{
    std::vector<glm::mat4> sceneObjectMatrices;
    std::vector<std::shared_ptr<SceneObject>> sceneObjects = scene->getSceneObjects(sceneObjectMatrices);
    if (sceneObjects.size() == 0)
    {
        utils::ConsoleWarning("Trying to ray trace an empty scene");
        return;
    }

    const SceneData& sceneData = scene->getSceneData();

    /* Create bottom level acceleration sturcutres out of all the meshes in the scene */
    for (size_t i = 0; i < sceneObjects.size(); i++)
    {
        auto so = sceneObjects[i];
        auto transform = sceneObjectMatrices[i];

        const VulkanMesh* mesh = reinterpret_cast<const VulkanMesh*>(so->get<Mesh *>(ComponentType::MESH));
        if (mesh != nullptr)
        {
            AccelerationStructure blas = createBottomLevelAccelerationStructure(*mesh, glm::mat4(1.0f));
            m_blas.push_back(std::make_pair(blas, transform));
        }
    }

    /* Create a top level accelleration structure out of all the blas */
    m_tlas = createTopLevelAccelerationStructure();

    updateUniformBuffers(sceneData);
    updateDescriptorSets();

    render();

    destroyAccellerationStructures();
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

bool VulkanRendererRayTracing::checkRayTracingSupport(VkPhysicalDevice device, std::vector<VkExtensionProperties>& availableExtensions)
{
    std::vector<const char *> requiredExtensions = VulkanRendererRayTracing::getRequiredExtensions();
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    availableExtensions = std::vector<VkExtensionProperties>(count);
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

VkDevice VulkanRendererRayTracing::createLogicalDevice(VkPhysicalDevice physicalDevice, uint32_t& queueFamilyIndex)
{
    /* Physical device features needed for ray tracing */
    VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures = {};
    physicalDeviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    physicalDeviceBufferDeviceAddressFeatures.pNext = NULL;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressCaptureReplay = VK_FALSE;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddressMultiDevice = VK_FALSE;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures = {};
    physicalDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    physicalDeviceAccelerationStructureFeatures.pNext = &physicalDeviceBufferDeviceAddressFeatures;
    physicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureCaptureReplay = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureIndirectBuild = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.accelerationStructureHostCommands = VK_FALSE;
    physicalDeviceAccelerationStructureFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = {};
    physicalDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    physicalDeviceRayTracingPipelineFeatures.pNext = &physicalDeviceAccelerationStructureFeatures; 
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect = VK_FALSE;
    physicalDeviceRayTracingPipelineFeatures.rayTraversalPrimitiveCulling = VK_FALSE;
    VkPhysicalDevice16BitStorageFeatures deviceFeatures16Storage = {};
    deviceFeatures16Storage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    deviceFeatures16Storage.storageBuffer16BitAccess = VK_TRUE;
    deviceFeatures16Storage.pNext = &physicalDeviceRayTracingPipelineFeatures;
    VkPhysicalDeviceFeatures2 deviceFeatures = {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.features.geometryShader = VK_TRUE;
    deviceFeatures.features.shaderInt64 = VK_TRUE;
    deviceFeatures.features.shaderInt16 = VK_TRUE;
    deviceFeatures.pNext = &deviceFeatures16Storage;

    /* Pick queue families */
    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, NULL);
    std::vector<VkQueueFamilyProperties> queueFamilyPropertiesList(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyPropertiesList.data());
    queueFamilyIndex = -1;
    for (uint32_t x = 0; x < queueFamilyPropertiesList.size(); x++) {
        if (queueFamilyPropertiesList[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = x;
            break;
        }
    }
    std::vector<float> queuePrioritiesList = { 1.0f };
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = NULL;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = queuePrioritiesList.data();

    /* Physical device required extensions */
    std::vector<const char*> requiredExtensions = VulkanRendererRayTracing::getRequiredExtensions();

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &deviceFeatures;
    deviceCreateInfo.flags = 0;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    deviceCreateInfo.enabledLayerCount = 0;
    deviceCreateInfo.ppEnabledLayerNames = NULL;
    deviceCreateInfo.enabledExtensionCount = (uint32_t)requiredExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
    deviceCreateInfo.pEnabledFeatures = VK_NULL_HANDLE;

    VkDevice device = VK_NULL_HANDLE;
    VkResult res = vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);

    if (res != VK_SUCCESS) {
        throw std::runtime_error("VulkanRayTracingRenderer::createLogicalDevice(): Unable to create logical device: " + std::to_string(res));
    }
    
    return device;
}

uint64_t VulkanRendererRayTracing::getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return m_devF.vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}

VulkanRendererRayTracing::AccelerationStructure VulkanRendererRayTracing::createBottomLevelAccelerationStructure(const VulkanMesh& mesh, const glm::mat4& t)
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

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    createBuffer(m_physicalDevice,
        m_device,
        vertices.size() * sizeof(Vertex),
        usageFlags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertices.data(),
        vertexBuffer,
        vertexBufferMemory);

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    createBuffer(m_physicalDevice,
        m_device,
        indices.size() * sizeof(indices[0]),
        usageFlags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        indices.data(),
        indexBuffer,
        indexBufferMemory);

    VkBuffer transformBuffer;
    VkDeviceMemory transformBufferMemory;
    createBuffer(m_physicalDevice,
        m_device,
        sizeof(transformMatrix),
        usageFlags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &transformMatrix,
        transformBuffer,
        transformBufferMemory);

    /* Get the device address of the buffers and push them to the scene objects vector */
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, vertexBuffer);
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, indexBuffer);
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, transformBuffer);
    m_sceneObjects.push_back({ vertexBufferDeviceAddress.deviceAddress, indexBufferDeviceAddress.deviceAddress });

    std::vector<uint32_t> numTriangles = { static_cast<uint32_t>(indices.size()) / 3 };
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
    createAccelerationStructureBuffer(m_physicalDevice,
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

    /* Get device address */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = blas.handle;
    blas.deviceAddress = m_devF.vkGetAccelerationStructureDeviceAddressKHR(m_device, &accelerationDeviceAddressInfo);

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
                                                                        is inside the array of references that is paased in the shader. 
                                                                        This reference will tell the shader where the geometry buffers 
                                                                        for this object are */
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = 0;    /* TODO Specify here the chit shader to be used from the SBT */
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = m_blas[i].first.deviceAddress;
    }
    uint32_t instancesCount = static_cast<uint32_t>(instances.size());

    /* Create buffer to copy the instances */
    VkBuffer instancesBuffer;
    VkDeviceMemory instancesBufferMemory;
    createBuffer(m_physicalDevice,
        m_device,
        instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        instances.data(),
        instancesBuffer,
        instancesBufferMemory);
    /* Get address of buffer */
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, instancesBuffer);

    /* Create an acceleration structure geometry to reference the instances buffer */
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;
    /* Get size info for the acceleration structure */
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

    /* Create acceleration sturcture buffer */
    AccelerationStructure tlas;
    createAccelerationStructureBuffer(m_physicalDevice,
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
    vkDestroyBuffer(m_device, instancesBuffer, nullptr);
    vkFreeMemory(m_device, instancesBufferMemory, nullptr);

    return tlas;
}

void VulkanRendererRayTracing::destroyAccellerationStructures()
{
    m_sceneObjects.clear();

    for (auto& blas : m_blas) {
        m_devF.vkDestroyAccelerationStructureKHR(m_device, blas.first.handle, nullptr);
    }
    m_blas.clear();

    m_devF.vkDestroyAccelerationStructureKHR(m_device, m_tlas.handle, nullptr);
}

void VulkanRendererRayTracing::createStorageImage()
{
    /* Create render target used during ray tracing */
    bool ret = createImage(m_physicalDevice,
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
    ret = createImage(m_physicalDevice,
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
    /* Create a buffer to hold the scene data */
    createBuffer(m_physicalDevice,
        m_device,
        sizeof(SceneData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBufferScene, 
        m_uniformBufferSceneMemory);

    /* TODO 100 max objects in the scene  */
    /* Create a buffer to hold the object descriptions data */
    createBuffer(m_physicalDevice,
        m_device,
        100u * sizeof(ObjectDescription),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        m_uniformBufferObjectDescription,
        m_uniformBufferObjectDescrptionMemory);
}

void VulkanRendererRayTracing::updateUniformBuffers(const SceneData& sceneData)
{
    /* BUffer holding scene data */
    {
        void* data;
        vkMapMemory(m_device, m_uniformBufferSceneMemory, 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(m_device, m_uniformBufferSceneMemory);
    }

    /* Buffer holding the description of geometry objects */
    {
        void* data;
        vkMapMemory(m_device, m_uniformBufferObjectDescrptionMemory, 0, VK_WHOLE_SIZE, 0, &data);
        memcpy(data, m_sceneObjects.data(), m_sceneObjects.size() * sizeof(ObjectDescription));
        vkUnmapMemory(m_device, m_uniformBufferObjectDescrptionMemory);
    }
}

void VulkanRendererRayTracing::createRayTracingPipeline()
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

    /* Binding 3, the object descriptions buffer */
    VkDescriptorSetLayoutBinding objectDescrptionBufferBinding{};
    objectDescrptionBufferBinding.binding = 3;
    objectDescrptionBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectDescrptionBufferBinding.descriptorCount = 1;
    objectDescrptionBufferBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

    std::vector<VkDescriptorSetLayoutBinding> bindings({
        accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
        sceneDataBufferBinding,
        objectDescrptionBufferBinding
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    res = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRayTracingRenderer::createRayTracingPipeline(): Failed to create a pipeline layout: " + std::to_string(res));
    }

    /*
        Setup ray tracing shader groups
    */
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    // Ray generation group
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

    // Miss group
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

    // Closest hit group
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
        shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;  /* Only chit shader used for this closest hit group */
        shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
        shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
        m_shaderGroups.push_back(shaderGroup);
    }

    // Spec only guarantees 1 level of "recursion". Check for that sad possibility here.
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
    res = m_devF.vkCreateRayTracingPipelinesKHR(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_pipeline);
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

    createBuffer(m_physicalDevice, m_device, handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayGenBuffer, m_shaderRayGenBufferMemory);
    createBuffer(m_physicalDevice, m_device, 2 * handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayMissBuffer, m_shaderRayMissBufferMemory);
    createBuffer(m_physicalDevice, m_device, handleSize, bufferUsageFlags, memoryUsageFlags, m_shaderRayCHitBuffer, m_shaderRayCHitBufferMemory);

    /* 1 ray gen group always */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayGenBufferMemory, 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data(), handleSize);
        vkUnmapMemory(m_device, m_shaderRayGenBufferMemory);
    }
    /* 2 ray miss groups */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayMissBufferMemory, 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
        vkUnmapMemory(m_device, m_shaderRayMissBufferMemory);
    }
    /* 1 ray chit group */
    {
        void* data;
        vkMapMemory(m_device, m_shaderRayCHitBufferMemory, 0, handleSize, 0, &data);
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
        vkUnmapMemory(m_device, m_shaderRayCHitBufferMemory);
    }
}


void VulkanRendererRayTracing::createDescriptorSets()
{
    /* First set is:
    * 1 accelleration structure
    * 1 storage image for output
    * 1 uniform buffer for scene data
    * 1 storage buffer for references to scene object geometry
    */
    std::vector<VkDescriptorPoolSize> poolSizes = {
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
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
    sceneDataDescriptor.buffer = m_uniformBufferScene;
    sceneDataDescriptor.offset = 0;
    sceneDataDescriptor.range = sizeof(SceneData);
    VkWriteDescriptorSet uniformBufferWrite{};
    uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uniformBufferWrite.dstSet = m_descriptorSet;
    uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniformBufferWrite.dstBinding = 2;
    uniformBufferWrite.pBufferInfo = &sceneDataDescriptor;
    uniformBufferWrite.descriptorCount = 1;

    /* Update object description binding */
    VkDescriptorBufferInfo objectDescriptionDataDesriptor{};
    objectDescriptionDataDesriptor.buffer = m_uniformBufferObjectDescription;
    objectDescriptionDataDesriptor.offset = 0;
    objectDescriptionDataDesriptor.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet objectDescriptionWrite{};
    objectDescriptionWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    objectDescriptionWrite.dstSet = m_descriptorSet;
    objectDescriptionWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objectDescriptionWrite.dstBinding = 3;
    objectDescriptionWrite.pBufferInfo = &objectDescriptionDataDesriptor;
    objectDescriptionWrite.descriptorCount = 1;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        accelerationStructureWrite,
        resultImageWrite,
        uniformBufferWrite,
        objectDescriptionWrite
    };
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void VulkanRendererRayTracing::render()
{
    VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkCommandBuffer cmdBuf = beginSingleTimeCommands(m_device, m_commandPool);

    /* Get strided device addresses for the shader binding tables where the shader group handles are stored. TODO get this out of the render function */
    const uint32_t handleSizeAligned = alignedSize(m_rayTracingPipelineProperties.shaderGroupHandleSize, m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayGenBuffer);
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;
    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayMissBuffer);
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = 2 * handleSizeAligned;
    VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
    hitShaderSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_shaderRayCHitBuffer);
    hitShaderSbtEntry.stride = handleSizeAligned;
    hitShaderSbtEntry.size = handleSizeAligned;
    /* No callable shader binding tables */
    VkStridedDeviceAddressRegionKHR emptySbtEntry{};
    
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, 0);
    
    /* Clear render result */
    VkClearColorValue clearColorValue = { { 0, 0, 0, 0} };
    vkCmdClearColorImage(
        cmdBuf,
        m_renderResult.image,
        VK_IMAGE_LAYOUT_GENERAL,
        &clearColorValue,
        1,
        &subresourceRange);
    
    /*
        Dispatch the ray tracing commands
    */
    m_devF.vkCmdTraceRaysKHR(
        cmdBuf,
        &raygenShaderSbtEntry,
        &missShaderSbtEntry,
        &hitShaderSbtEntry,
        &emptySbtEntry,
        m_width,
        m_height,
        1);
    
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

    endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf, true);
    vkQueueWaitIdle(m_queue);

    storeToDisk("test", OutputFileType::HDR);
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
    case VulkanRendererRayTracing::OutputFileType::PNG:
    {
        /* TODO apply exposure */
        std::vector<unsigned char> imageDataInt(m_width * m_height * 4, 255);
        for (size_t i = 0; i < m_width * m_height * 4; i++)
        {
            imageDataInt[i] = imageDataFloat[i] * 255.0f;
        }
        stbi_write_png((filename + ".png").c_str(), m_width, m_height, 4, imageDataInt.data(), m_width * 4 * sizeof(char));
        break;
    }
    case VulkanRendererRayTracing::OutputFileType::HDR:
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
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

