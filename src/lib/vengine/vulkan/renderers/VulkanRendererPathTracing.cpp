#include "VulkanRendererPathTracing.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <sys/types.h>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <debug_tools/Console.hpp>
#include <debug_tools/Timer.hpp>

#include "core/Materials.hpp"
#include "utils/ECS.hpp"
#include "utils/ImageUtils.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/resources/VulkanLight.hpp"

namespace vengine
{

VulkanRendererPathTracing::VulkanRendererPathTracing(VulkanContext &vkctx,
                                                     VulkanMaterials &materials,
                                                     VulkanTextures &textures,
                                                     VulkanRandom &random)
    : m_vkctx(vkctx)
    , m_materials(materials)
    , m_textures(textures)
    , m_random(random)
{
}

VkResult VulkanRendererPathTracing::initResources(VkFormat format, VkDescriptorSetLayout skyboxDescriptorLayout)
{
    /* Check if physical device supports ray tracing */
    bool supportsRayTracing = VulkanRendererPathTracing::checkRayTracingSupport(m_vkctx.physicalDevice());
    if (!supportsRayTracing) {
        throw std::runtime_error("Ray tracing is not supported");
    }

    m_format = format;
    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;

    m_physicalDeviceProperties = m_vkctx.physicalDeviceProperties();

    m_device = m_vkctx.device();
    m_commandPool = m_vkctx.graphicsCommandPool();
    m_queue = m_vkctx.graphicsQueue();

    /* Get ray tracing specific device functions pointers */
    {
        m_devF.vkGetBufferDeviceAddressKHR =
            reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_device, "vkGetBufferDeviceAddressKHR"));
        m_devF.vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr(m_device, "vkCmdBuildAccelerationStructuresKHR"));
        m_devF.vkBuildAccelerationStructuresKHR =
            reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_device, "vkBuildAccelerationStructuresKHR"));
        m_devF.vkCreateAccelerationStructureKHR =
            reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_device, "vkCreateAccelerationStructureKHR"));
        m_devF.vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
            vkGetDeviceProcAddr(m_device, "vkDestroyAccelerationStructureKHR"));
        m_devF.vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureBuildSizesKHR"));
        m_devF.vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(m_device, "vkGetAccelerationStructureDeviceAddressKHR"));
        m_devF.vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_device, "vkCmdTraceRaysKHR"));
        m_devF.vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
            vkGetDeviceProcAddr(m_device, "vkGetRayTracingShaderGroupHandlesKHR"));
        m_devF.vkCreateRayTracingPipelinesKHR =
            reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_device, "vkCreateRayTracingPipelinesKHR"));
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

    VULKAN_CHECK_CRITICAL(createBuffers());
    VULKAN_CHECK_CRITICAL(createRayTracingPipeline());
    VULKAN_CHECK_CRITICAL(createShaderBindingTable());
    VULKAN_CHECK_CRITICAL(createDescriptorSets());

    m_isInitialized = true;
    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::releaseRenderResources()
{
    for (auto &mb : m_renderBuffers) {
        mb.destroy(m_device);
    }
    m_renderBuffers.clear();

    destroyAccellerationStructures();

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::releaseResources()
{
    vkDestroyPipeline(m_device, m_pipeline, nullptr);

    vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    m_sbt.raygenBuffer.destroy(m_device);
    m_sbt.raymissBuffer.destroy(m_device);
    m_sbt.raychitBuffer.destroy(m_device);

    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayoutMain, nullptr);

    m_uniformBufferScene.destroy(m_device);
    m_storageBufferObjectDescription.destroy(m_device);

    m_tempImage.destroy(m_device);
    m_renderResult.destroy(m_device);

    return VK_SUCCESS;
}

bool VulkanRendererPathTracing::isRayTracingEnabled() const
{
    return m_isInitialized;
}

void VulkanRendererPathTracing::render(const Scene &scene)
{
    if (m_renderInProgress)
        return;

    m_renderProgress = 0.0F;

    debug_tools::Timer timer;
    timer.Start();

    /* Get the scene objects */
    std::vector<glm::mat4> sceneObjectMatrices;
    SceneGraph sceneObjects = scene.getSceneObjectsArray(sceneObjectMatrices);
    if (sceneObjects.size() == 0) {
        debug_tools::ConsoleWarning("Trying to render an empty scene");
        return;
    }

    m_renderInProgress = true;

    /* Prepare sceneData */
    SceneData sceneData = scene.getSceneData();
    {
        uint32_t width = renderInfo().width;
        uint32_t height = renderInfo().height;

        /* Change to requested resolution */
        auto camera = scene.camera();
        switch (camera->type()) {
            case CameraType::PERSPECTIVE: {
                auto perspectiveCamera = std::static_pointer_cast<PerspectiveCamera>(camera);
                sceneData.m_projection =
                    glm::perspective(glm::radians(perspectiveCamera->fov()), static_cast<float>(width) / height, 0.01f, 200.0f);
                sceneData.m_projection[1][1] *= -1;
                sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);
                break;
            }
            case CameraType::ORTHOGRAPHIC: {
                auto orthoCamera = std::static_pointer_cast<OrthographicCamera>(camera);
                auto owidth = static_cast<float>(width);
                auto oheight = static_cast<float>(height);
                sceneData.m_projection = glm::ortho(-owidth / 2, owidth / 2, -oheight / 2, oheight / 2, -100.0f, 100.0f);
                sceneData.m_projection[1][1] *= -1;
                sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);
            }
            default:
                break;
        }
    }

    std::vector<LightPT> sceneLights;

    /* Create bottom level acceleration sturcutres out of all the meshes in the scene */
    for (size_t i = 0; i < sceneObjects.size(); i++) {
        auto so = sceneObjects[i];
        auto transform = sceneObjectMatrices[i];
        if (so->has<ComponentMesh>() && so->has<ComponentMaterial>()) {
            auto mesh = static_cast<VulkanMesh *>(so->get<ComponentMesh>().mesh);
            auto material = so->get<ComponentMaterial>().material;

            AccelerationStructure blas = createBottomLevelAccelerationStructure(*mesh, glm::mat4(1.0f), material);

            uint32_t nTypeRays = 3U; /* Types of rays */
            uint32_t instanceOffset; /* Ioffset in SBT is based on the type of material */
            if (material->type() == MaterialType::MATERIAL_LAMBERT)
                instanceOffset = 0 * nTypeRays;
            else if (material->type() == MaterialType::MATERIAL_PBR_STANDARD)
                instanceOffset = 1 * nTypeRays;
            else {
                throw std::runtime_error("VulkanRendererPathTracing::renderScene(): Unexpected material");
            }

            m_blas.emplace_back(blas, transform, instanceOffset);
        }

        if (so->has<ComponentLight>() || isMeshLight(so)) {
            prepareSceneObjectLight(so, m_sceneObjectsDescription.size() - 1, transform, sceneLights);
        }
    }

    /* Create a top level accelleration structure out of all the blas */
    m_tlas = createTopLevelAccelerationStructure();

    /* Prepare scene lights */
    prepareSceneLights(scene, sceneLights);
    m_pathTracingData.lights.r = sceneLights.size();
    updateBuffers(sceneData, m_pathTracingData, sceneLights);

    /* We will use the materials from buffer index 0 for this renderer */
    m_materials.updateBuffers(0);

    /* Get skybox descriptor */
    auto skybox = dynamic_cast<const VulkanMaterialSkybox *>(scene.skyboxMaterial());
    assert(skybox != nullptr);

    setResolution();
    render(skybox->getDescriptor(0));

    releaseRenderResources();

    timer.Stop();

    std::string fileExtension = (renderInfo().fileType == FileType::PNG) ? "png" : "hdr";
    std::string filename = renderInfo().filename + "." + fileExtension;
    debug_tools::ConsoleInfo("Scene rendered: " + filename + " in: " + std::to_string(timer.ToInt()) + "ms");

    m_renderInProgress = false;
}

float VulkanRendererPathTracing::renderProgress()
{
    return m_renderProgress;
}

void VulkanRendererPathTracing::setResolution()
{
    m_renderResult.destroy(m_device);
    m_tempImage.destroy(m_device);

    createStorageImage(renderInfo().width, renderInfo().height);
}

std::vector<const char *> VulkanRendererPathTracing::getRequiredExtensions()
{
    return {"VK_KHR_acceleration_structure",
            "VK_KHR_ray_tracing_pipeline",
            "VK_KHR_buffer_device_address",
            "VK_KHR_deferred_host_operations",
            "VK_EXT_descriptor_indexing",
            "VK_KHR_spirv_1_4",
            "VK_KHR_shader_float_controls",
            "VK_KHR_maintenance3"};
}

bool VulkanRendererPathTracing::checkRayTracingSupport(VkPhysicalDevice device)
{
    /* Get required extensions */
    std::vector<const char *> requiredExtensions = VulkanRendererPathTracing::getRequiredExtensions();

    /* Get the number of extensions supported by the device */
    uint32_t count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

    /* Check if the required extensions are supported */
    auto availableExtensions = std::vector<VkExtensionProperties>(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.data());  // populate buffer
    for (auto requiredExetension : requiredExtensions) {
        bool found = false;
        for (auto &availableExtension : availableExtensions) {
            if (strcmp(requiredExetension, availableExtension.extensionName) == 0) {
                found = true;
                continue;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

uint64_t VulkanRendererPathTracing::getBufferDeviceAddress(VkDevice device, VkBuffer buffer)
{
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return m_devF.vkGetBufferDeviceAddressKHR(device, &bufferDeviceAI);
}

VulkanRendererPathTracing::AccelerationStructure
VulkanRendererPathTracing::createBottomLevelAccelerationStructure(const VulkanMesh &mesh, const glm::mat4 &t, const Material *material)
{
    // TODO compute BLAS per VulkanMesh object

    const std::vector<Vertex> &vertices = mesh.vertices();
    const std::vector<uint32_t> indices = mesh.indices();
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    VkTransformMatrixKHR transformMatrix = {
        t[0][0], t[1][0], t[2][0], t[3][0], t[0][1], t[1][1], t[2][1], t[3][1], t[0][2], t[1][2], t[2][2], t[3][2]};

    /* Create buffers for RT for mesh data and the transformation matrix */
    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    VkBuffer vertexBuffer = mesh.vertexBuffer().buffer();
    VkDeviceMemory vertexBufferMemory = mesh.vertexBuffer().memory();

    VkBuffer indexBuffer = mesh.indexBuffer().buffer();
    VkDeviceMemory indexBufferMemory = mesh.indexBuffer().memory();

    // TODO use transform buffer from the dybamic UBO that stores the mvp matrices?
    VulkanBuffer transformBuffer;
    createBuffer(m_vkctx.physicalDevice(),
                 m_device,
                 sizeof(transformMatrix),
                 usageFlags,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &transformMatrix,
                 transformBuffer);

    /* Get the device address of the geometry buffers and push them to the scene objects vector */
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, vertexBuffer);
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, indexBuffer);
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, transformBuffer.buffer());

    std::vector<uint32_t> numTriangles = {indexCount / 3};
    uint32_t maxVertex = static_cast<uint32_t>(vertices.size());

    m_sceneObjectsDescription.push_back(
        {vertexBufferDeviceAddress.deviceAddress, indexBufferDeviceAddress.deviceAddress, material->materialIndex(), numTriangles[0]});

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
    m_devF.vkGetAccelerationStructureBuildSizesKHR(m_device,
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
    ScratchBuffer scratchBuffer;
    createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize, scratchBuffer);

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
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
        &accelerationStructureBuildRangeInfo};

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer;
    beginSingleTimeCommands(m_device, m_commandPool, commandBuffer);

    m_devF.vkCmdBuildAccelerationStructuresKHR(
        commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());

    endSingleTimeCommands(m_device, m_commandPool, m_queue, commandBuffer);

    /* Get the device address */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = blas.handle;
    blas.deviceAddress = m_devF.vkGetAccelerationStructureDeviceAddressKHR(m_device, &accelerationDeviceAddressInfo);

    m_renderBuffers.push_back(transformBuffer);
    deleteScratchBuffer(scratchBuffer);

    return blas;
}

VulkanRendererPathTracing::AccelerationStructure VulkanRendererPathTracing::createTopLevelAccelerationStructure()
{
    /* Create a buffer to hold top level instances */
    std::vector<VkAccelerationStructureInstanceKHR> instances(m_blas.size());
    for (size_t i = 0; i < m_blas.size(); i++) {
        auto &t = m_blas[i].transform;
        /* Set transform matrix per blas */
        VkTransformMatrixKHR transformMatrix = {
            t[0][0], t[1][0], t[2][0], t[3][0], t[0][1], t[1][1], t[2][1], t[3][1], t[0][2], t[1][2], t[2][2], t[3][2]};

        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = static_cast<uint32_t>(i); /* The custom index specifies where the reference of this object
                                                                     is inside the array of references that is passed in the shader.
                                                                     More specifically, this reference will tell the shader where the
                                                                     geometry buffers for this object are, the mesh's position inside
                                                                     the ObjectDescription buffer */
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = m_blas[i].instanceOffset;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = m_blas[i].as.deviceAddress;
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
    instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(m_device, instancesBuffer.buffer());

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
    m_devF.vkGetAccelerationStructureBuildSizesKHR(m_device,
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
    ScratchBuffer scratchBuffer;
    createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize, scratchBuffer);

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
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> accelerationBuildStructureRangeInfos = {
        &accelerationStructureBuildRangeInfo};

    /* Build the acceleration structure on the device via a one - time command buffer submission */
    VkCommandBuffer commandBuffer;
    beginSingleTimeCommands(m_device, m_commandPool, commandBuffer);

    m_devF.vkCmdBuildAccelerationStructuresKHR(
        commandBuffer, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos.data());

    endSingleTimeCommands(m_device, m_commandPool, m_queue, commandBuffer);

    /* Get device address of accelleration structure */
    VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
    accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationDeviceAddressInfo.accelerationStructure = tlas.handle;
    tlas.deviceAddress = m_devF.vkGetAccelerationStructureDeviceAddressKHR(m_device, &accelerationDeviceAddressInfo);

    deleteScratchBuffer(scratchBuffer);
    m_renderBuffers.push_back(instancesBuffer);

    return tlas;
}

void VulkanRendererPathTracing::destroyAccellerationStructures()
{
    m_sceneObjectsDescription.clear();

    for (auto &blas : m_blas) {
        m_devF.vkDestroyAccelerationStructureKHR(m_device, blas.as.handle, nullptr);
        vkDestroyBuffer(m_device, blas.as.buffer, nullptr);
        vkFreeMemory(m_device, blas.as.memory, nullptr);
    }
    m_blas.clear();

    m_devF.vkDestroyAccelerationStructureKHR(m_device, m_tlas.handle, nullptr);
    vkDestroyBuffer(m_device, m_tlas.buffer, nullptr);
    vkFreeMemory(m_device, m_tlas.memory, nullptr);
}

VkResult VulkanRendererPathTracing::createStorageImage(uint32_t width, uint32_t height)
{
    /* Create render target used during path tracing */
    VkImageCreateInfo imageInfo1 =
        vkinit::imageCreateInfo({width, height, 1},
                                m_format,
                                1,
                                VK_SAMPLE_COUNT_1_BIT,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                      m_device,
                                      imageInfo1,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      m_renderResult.image,
                                      m_renderResult.memory));

    VkImageViewCreateInfo imageInfo1View = vkinit::imageViewCreateInfo(m_renderResult.image, m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &imageInfo1View, nullptr, &m_renderResult.view));

    /* Create temp image used to copy render result from gpu memory to cpu memory */
    VkImageCreateInfo imageInfo2 = vkinit::imageCreateInfo(
        {width, height, 1}, m_format, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                      m_device,
                                      imageInfo2,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                      m_tempImage.image,
                                      m_tempImage.memory));

    /* Transition images to the approriate layout ready for render */
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_device, m_commandPool, cmdBuf));

    transitionImageLayout(
        cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    transitionImageLayout(cmdBuf,
                          m_tempImage.image,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf));

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createBuffers()
{
    /* The scene buffer holds [SceneData | PathTracingData | VULKAN_LIMITS_MAX_LIGHTS * LightPT ] */
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t totalSceneBufferSize = alignedSize(sizeof(SceneData), alignment) + alignedSize(sizeof(PathTracingData), alignment) +
                                    alignedSize(VULKAN_LIMITS_MAX_LIGHTS * sizeof(LightPT), alignment);

    /* Create a buffer to hold the scene buffer */
    VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                       m_device,
                                       totalSceneBufferSize,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       m_uniformBufferScene));

    /* Create a buffer to hold the object descriptions data */
    VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                       m_device,
                                       VULKAN_LIMITS_MAX_OBJECTS * sizeof(ObjectDescriptionPT),
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       m_storageBufferObjectDescription));

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::updateBuffers(const SceneData &sceneData,
                                                  const PathTracingData &rtData,
                                                  const std::vector<LightPT> &lights)
{
    if (m_sceneObjectsDescription.size() > VULKAN_LIMITS_MAX_OBJECTS) {
        throw std::runtime_error("VulkanRendererPathTracing::updateUniformBuffers(): Number of objects exceeded");
    }
    if (lights.size() > VULKAN_LIMITS_MAX_LIGHTS) {
        throw std::runtime_error("VulkanRendererPathTracing::updateUniformBuffers(): Number of lights exceeded");
    }

    /* Update scene buffer */
    {
        VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_uniformBufferScene.memory(), 0, VK_WHOLE_SIZE, 0, &data));
        memcpy(data, &sceneData, sizeof(sceneData)); /* Copy scene data struct */
        memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment),
               &rtData,
               sizeof(PathTracingData)); /* Copy path tracing data struct */
        memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment) + alignedSize(sizeof(PathTracingData), alignment),
               lights.data(),
               lights.size() * sizeof(LightPT)); /* Copy lights vector */
        vkUnmapMemory(m_device, m_uniformBufferScene.memory());
    }

    /* Update description of geometry objects */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_storageBufferObjectDescription.memory(), 0, VK_WHOLE_SIZE, 0, &data));
        memcpy(data, m_sceneObjectsDescription.data(), m_sceneObjectsDescription.size() * sizeof(ObjectDescriptionPT));
        vkUnmapMemory(m_device, m_storageBufferObjectDescription.memory());
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::updateBuffersPathTracingData(const PathTracingData &ptData)
{
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

    void *data;
    VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_uniformBufferScene.memory(), 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), alignment),
           &ptData,
           sizeof(PathTracingData)); /* Copy path tracing data struct */

    vkUnmapMemory(m_device, m_uniformBufferScene.memory());

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createDescriptorSets()
{
    /* First set is:
     * 1 accelleration structure
     * 1 storage image for output
     * 1 uniform buffer for scene data
     * 1 storage buffer for references to scene object geometry
     * 1 uniform buffer for light data
     */
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
    };
    VkDescriptorPoolCreateInfo descriptorPoolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_device, &descriptorPoolInfo, nullptr, &m_descriptorPool));

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
        vkinit::descriptorSetAllocateInfo(m_descriptorPool, 1, &m_descriptorSetLayoutMain);
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet));

    return VK_SUCCESS;
}

void VulkanRendererPathTracing::updateDescriptorSets()
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
    VkDescriptorImageInfo storageImageDescriptor =
        vkinit::descriptorImageInfo(VK_NULL_HANDLE, m_renderResult.view, VK_IMAGE_LAYOUT_GENERAL);
    VkWriteDescriptorSet resultImageWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, 1, &storageImageDescriptor);

    /* Update scene data binding */
    VkDescriptorBufferInfo sceneDataDescriptor = vkinit::descriptorBufferInfo(m_uniformBufferScene.buffer(), 0, sizeof(SceneData));
    VkWriteDescriptorSet sceneDataWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, 1, &sceneDataDescriptor);

    /* Update path tracing data binding, uses the same buffer with the scene data */
    VkDescriptorBufferInfo pathTracingDataDescriptor = vkinit::descriptorBufferInfo(
        m_uniformBufferScene.buffer(), alignedSize(sizeof(SceneData), uniformBufferAlignment), sizeof(PathTracingData));
    VkWriteDescriptorSet pathTracingDataWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, 1, &pathTracingDataDescriptor);

    /* Update object description binding */
    VkDescriptorBufferInfo objectDescriptionDataDesriptor =
        vkinit::descriptorBufferInfo(m_storageBufferObjectDescription.buffer(), 0, VK_WHOLE_SIZE);
    VkWriteDescriptorSet objectDescriptionWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, 1, &objectDescriptionDataDesriptor);

    /* Update lights buffer binding */
    VkDescriptorBufferInfo lightsDescriptor = vkinit::descriptorBufferInfo(
        m_uniformBufferScene.buffer(),
        alignedSize(sizeof(SceneData), uniformBufferAlignment) + alignedSize(sizeof(PathTracingData), uniformBufferAlignment),
        alignedSize(VULKAN_LIMITS_MAX_LIGHTS * sizeof(LightPT), uniformBufferAlignment));
    VkWriteDescriptorSet lightsWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, 1, &lightsDescriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        accelerationStructureWrite, resultImageWrite, sceneDataWrite, pathTracingDataWrite, objectDescriptionWrite, lightsWrite};
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

VkResult VulkanRendererPathTracing::createRayTracingPipeline()
{
    /* Create the main descriptor set */
    {
        /* binding 0, the accelleration strucure */
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 1);
        /* Binding 1, the output image */
        VkDescriptorSetLayoutBinding resultImageLayoutBinding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1, 1);
        /* Binding 2, the scene data */
        VkDescriptorSetLayoutBinding sceneDataBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
            2,
            1);
        /* Binding 3, the path tracing data */
        VkDescriptorSetLayoutBinding pathTracingDataBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
            3,
            1);
        /* Binding 4, the object descriptions buffer */
        VkDescriptorSetLayoutBinding objectDescrptionBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, 4, 1);
        /* Binding 5, the pt lights buffer */
        VkDescriptorSetLayoutBinding lightsBufferBinding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 5, 1);
        std::vector<VkDescriptorSetLayoutBinding> bindings({accelerationStructureLayoutBinding,
                                                            resultImageLayoutBinding,
                                                            sceneDataBufferBinding,
                                                            pathTracingDataBufferBinding,
                                                            objectDescrptionBufferBinding,
                                                            lightsBufferBinding});

        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo =
            vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorSetlayoutInfo, nullptr, &m_descriptorSetLayoutMain));
    }

    /* Create pipeline layout */
    {
        std::array<VkDescriptorSetLayout, 4> setsLayouts = {m_descriptorSetLayoutMain,
                                                            m_materials.descriptorSetLayout(),
                                                            m_textures.descriptorSetLayout(),
                                                            m_descriptorSetLayoutSkybox};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            vkinit::pipelineLayoutCreateInfo(static_cast<uint32_t>(setsLayouts.size()), setsLayouts.data(), 0, nullptr);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
    }

    /*
        Setup path tracing shader groups
    */
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    /* Ray generation group */
    {
        VkPipelineShaderStageCreateInfo rayGenShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_RAYGEN_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raygen.rgen.spv"), "main");
        shaderStages.push_back(rayGenShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                                       /* generalShader = */ static_cast<uint32_t>(shaderStages.size()) - 1,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR);
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Miss groups */
    {
        /* Primary ray miss */
        VkPipelineShaderStageCreateInfo rayMissShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_MISS_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raymissPrimary.rmiss.spv"), "main");
        shaderStages.push_back(rayMissShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                                                       /* generalShader = */ static_cast<uint32_t>(shaderStages.size()) - 1,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR);
        m_shaderGroups.push_back(shaderGroup);

        /* Secondary ray miss */
        VkPipelineShaderStageCreateInfo secondaryMissShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_MISS_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raymissSecondary.rmiss.spv"), "main");
        shaderStages.push_back(secondaryMissShaderStageInfo);
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        m_shaderGroups.push_back(shaderGroup);

        /* NEE ray miss */
        VkPipelineShaderStageCreateInfo neeMissShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_MISS_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raymissNEE.rmiss.spv"), "main");
        shaderStages.push_back(neeMissShaderStageInfo);
        shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Primary ray any hit shader */
    VkPipelineShaderStageCreateInfo rayPrimaryAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitPrimary.rahit.spv"), "main");
    shaderStages.push_back(rayPrimaryAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitPrimary = static_cast<uint32_t>(shaderStages.size()) - 1;

    /* Secondary ray any hit shader */
    VkPipelineShaderStageCreateInfo raySecondaryAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitSecondary.rahit.spv"), "main");
    shaderStages.push_back(raySecondaryAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitSecondary = static_cast<uint32_t>(shaderStages.size()) - 1;

    /* NEE any hit shader */
    VkPipelineShaderStageCreateInfo rayNEEAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitNEE.rahit.spv"), "main");
    shaderStages.push_back(rayNEEAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitNEE = static_cast<uint32_t>(shaderStages.size()) - 1;

    /* Hit group, lambert material, Primary ray */
    {
        VkPipelineShaderStageCreateInfo rayClosestHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raychitLambert.rchit.spv"), "main");
        shaderStages.push_back(rayClosestHitShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ static_cast<uint32_t>(shaderStages.size()) - 1,
                                                       /* anyHitShader = */ shaderStageAnyHitPrimary);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, lambert material, Secondary ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitSecondary);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, lambert material, NEE ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitNEE);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, PBRStandard material, Primary ray */
    {
        VkPipelineShaderStageCreateInfo rayClosestHitShaderStageInfo =
            vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
                                                  VulkanShader::load(m_device, "shaders/SPIRV/pt/raychitPBRStandard.rchit.spv"),
                                                  "main");
        shaderStages.push_back(rayClosestHitShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ static_cast<uint32_t>(shaderStages.size()) - 1,
                                                       /* anyHitShader = */ shaderStageAnyHitPrimary);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, PBRStandard material, Secondary ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitSecondary);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, PBRStandard material, NEE ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitNEE);
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Spec only guarantees 1 level of "recursion". Check for that sad possibility here */
    if (m_rayTracingPipelineProperties.maxRayRecursionDepth <= 1) {
        throw std::runtime_error("VulkanRendererPathTracing::createRayTracingPipeline(): Device doesn't support ray recursion");
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
    VULKAN_CHECK_CRITICAL(m_devF.vkCreateRayTracingPipelinesKHR(
        m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &m_pipeline));

    /* Destroy shader modules */
    for (auto &s : shaderStages) {
        vkDestroyShaderModule(m_device, s.module, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createShaderBindingTable()
{
    const uint32_t handleSize = m_rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned =
        alignedSize(m_rayTracingPipelineProperties.shaderGroupHandleSize, m_rayTracingPipelineProperties.shaderGroupHandleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(m_shaderGroups.size());
    const uint32_t sbtSize = groupCount * handleSizeAligned; /* Size in bytes */

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    VkResult res =
        m_devF.vkGetRayTracingShaderGroupHandlesKHR(m_device, m_pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("VulkanRendererPathTracing::createShaderBindingTable(): Failed to get the shader group handles: " +
                                 std::to_string(res));
    }

    /* Create shader binding tables for each group, ray gen, miss and chit, and store the shader group handles */
    const VkBufferUsageFlags bufferUsageFlags =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    /* 1 gen group, 3 miss groups, 6 hit groups */
    VULKAN_CHECK_CRITICAL(
        createBuffer(m_vkctx.physicalDevice(), m_device, handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raygenBuffer));
    VULKAN_CHECK_CRITICAL(
        createBuffer(m_vkctx.physicalDevice(), m_device, 3 * handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raymissBuffer));
    VULKAN_CHECK_CRITICAL(
        createBuffer(m_vkctx.physicalDevice(), m_device, 6 * handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raychitBuffer));

    /* gen group */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raygenBuffer.memory(), 0, handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data(), handleSize);
        vkUnmapMemory(m_device, m_sbt.raygenBuffer.memory());
    }
    /* miss groups */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raymissBuffer.memory(), 0, 3 * handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned, 3 * handleSize);
        vkUnmapMemory(m_device, m_sbt.raymissBuffer.memory());
    }
    /* hit groups */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raychitBuffer.memory(), 0, 6 * handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data() + handleSizeAligned * 4, 6 * handleSize);
        vkUnmapMemory(m_device, m_sbt.raychitBuffer.memory());
    }

    /* Get strided device addresses for the shader binding tables where the shader group handles are stored */
    m_sbt.raygenSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raygenBuffer.buffer());
    m_sbt.raygenSbtEntry.stride = handleSizeAligned;
    m_sbt.raygenSbtEntry.size = handleSizeAligned;
    m_sbt.raymissSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raymissBuffer.buffer());
    m_sbt.raymissSbtEntry.stride = handleSizeAligned;
    m_sbt.raymissSbtEntry.size = 3 * handleSizeAligned;
    m_sbt.raychitSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raychitBuffer.buffer());
    m_sbt.raychitSbtEntry.stride = handleSizeAligned;
    m_sbt.raychitSbtEntry.size = 6 * handleSizeAligned;
    m_sbt.callableSbtEntry = {};

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::render(VkDescriptorSet skyboxDescriptor)
{
    m_pathTracingData.samplesBatchesDepthIndex.r = renderInfo().samples;
    m_pathTracingData.samplesBatchesDepthIndex.b = renderInfo().depth;

    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    uint32_t batchSize = renderInfo().batchSize;
    uint32_t batches = renderInfo().samples / batchSize;

    VkCommandBuffer commandBuffer;
    {
        VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, 1);
        VULKAN_CHECK_CRITICAL(vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer));
    }

    VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo();
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    debug_tools::ConsoleInfo("Launching render with: " + std::to_string(batches * batchSize) + " samples");
    for (uint32_t batch = 0; batch < batches; batch++) {
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        PathTracingData batchData = m_pathTracingData;
        batchData.samplesBatchesDepthIndex.r = batchSize;
        batchData.samplesBatchesDepthIndex.g = batches;
        batchData.samplesBatchesDepthIndex.a = batch;
        VULKAN_CHECK_CRITICAL(updateBuffersPathTracingData(batchData));
        updateDescriptorSets();

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);

        std::array<VkDescriptorSet, 4> descriptorSets = {
            m_descriptorSet, m_materials.descriptorSet(0), m_textures.descriptorSet(), skyboxDescriptor};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                                m_pipelineLayout,
                                0,
                                static_cast<uint32_t>(descriptorSets.size()),
                                descriptorSets.data(),
                                0,
                                nullptr);

        /*
            Dispatch the ray tracing commands
        */
        m_devF.vkCmdTraceRaysKHR(commandBuffer,
                                 &m_sbt.raygenSbtEntry,
                                 &m_sbt.raymissSbtEntry,
                                 &m_sbt.raychitSbtEntry,
                                 &m_sbt.callableSbtEntry,
                                 renderInfo().width,
                                 renderInfo().height,
                                 1);

        vkEndCommandBuffer(commandBuffer);

        /* Create fence to wait for the command to finish */
        VkFenceCreateInfo fenceCreateInfo = vkinit::fenceCreateInfo();
        VkFence fence;
        VULKAN_CHECK_CRITICAL(vkCreateFence(m_device, &fenceCreateInfo, nullptr, &fence));

        /* Submit command buffer */
        VkSubmitInfo submitInfo = vkinit::submitInfo(1, &commandBuffer);
        VULKAN_CHECK_CRITICAL(vkQueueSubmit(m_queue, 1, &submitInfo, fence));

        /* Wait for the fence to signal that the command buffer has finished executing */
        VkResult res = vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_1S);
        if (res == VK_SUCCESS) {
            vkDestroyFence(m_device, fence, nullptr);
        } else {
            debug_tools::ConsoleCritical("VulkanRendererPathTracing::render(): Render failed: " + std::to_string(res));
        }

        m_renderProgress = static_cast<float>(batch) / static_cast<float>(batches);
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);

    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_device, m_commandPool, cmdBuf));

    /* Transition output render result to transfer source optimal */
    transitionImageLayout(
        cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    /* Copy render result to temp image */
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {renderInfo().width, renderInfo().height, 1};
    vkCmdCopyImage(cmdBuf,
                   m_renderResult.image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_tempImage.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &copyRegion);

    /* Transition render result back to layout general */
    transitionImageLayout(
        cmdBuf, m_renderResult.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);

    VkResult res = endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf, true, VULKAN_TIMEOUT_100S);
    if (res != VK_SUCCESS) {
        debug_tools::ConsoleCritical("VulkanRendererPathTracing::render(): Storing failed: " + std::to_string(res));
    } else {
        storeToDisk(renderInfo().filename, renderInfo().fileType);
    }

    VULKAN_CHECK_CRITICAL(vkQueueWaitIdle(m_queue));

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::storeToDisk(std::string filename, FileType type) const
{
    /* Store result from temp image to the disk */
    if (m_format != VK_FORMAT_R32G32B32A32_SFLOAT) {
        std::string error = "VulkanPathTracing::StoreToDisk(): Format supported is VK_FORMAT_R32G32B32A32_SFLOAT";
        debug_tools::ConsoleCritical(error);
        throw std::runtime_error(error);
    }

    float *input;
    VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_tempImage.memory, 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void **>(&input)));

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_device, m_tempImage.image, &subResource, &subResourceLayout);
    uint32_t rowPitchFloats = subResourceLayout.rowPitch / sizeof(float);

    uint32_t width = renderInfo().width;
    uint32_t height = renderInfo().height;
    std::vector<float> imageDataFloat(width * height * 4);
    uint32_t indexGPUIMage = subResourceLayout.offset;
    uint32_t indexOutputImage = 0;
    for (uint32_t i = 0; i < height; i++) {
        memcpy(&imageDataFloat[0] + indexOutputImage, &input[0] + indexGPUIMage, 4 * width * sizeof(float));
        indexGPUIMage += rowPitchFloats;
        indexOutputImage += 4 * width;
    }
    vkUnmapMemory(m_device, m_tempImage.memory);

    switch (type) {
        case FileType::PNG: {
            /* TODO apply exposure */
            std::vector<unsigned char> imageDataInt(width * height * 4, 255);
            for (size_t i = 0; i < width * height * 4; i++) {
                imageDataInt[i] = 255.F * linearToSRGB(std::clamp(imageDataFloat[i], 0.0F, 1.0F));
            }
            stbi_write_png((filename + ".png").c_str(), width, height, 4, imageDataInt.data(), width * 4 * sizeof(char));
            break;
        }
        case FileType::HDR: {
            stbi_write_hdr((filename + ".hdr").c_str(), width, height, 4, imageDataFloat.data());
            break;
        }
        default:
            debug_tools::ConsoleWarning("VulkanPathTracing::StoreToDisk(): Unknown output file type");
            break;
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createScratchBuffer(VkDeviceSize size, ScratchBuffer &scratchBuffer)
{
    scratchBuffer = {};

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    VULKAN_CHECK_CRITICAL(vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &scratchBuffer.handle));

    VkMemoryRequirements memoryRequirements{};
    vkGetBufferMemoryRequirements(m_device, scratchBuffer.handle, &memoryRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo memoryAllocateInfo = {};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex =
        findMemoryType(m_vkctx.physicalDevice(), memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VULKAN_CHECK_CRITICAL(vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &scratchBuffer.memory));

    VULKAN_CHECK_CRITICAL(vkBindBufferMemory(m_device, scratchBuffer.handle, scratchBuffer.memory, 0));

    VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = scratchBuffer.handle;
    scratchBuffer.deviceAddress = m_devF.vkGetBufferDeviceAddressKHR(m_device, &bufferDeviceAddressInfo);

    return VK_SUCCESS;
}

void VulkanRendererPathTracing::deleteScratchBuffer(ScratchBuffer &scratchBuffer)
{
    if (scratchBuffer.memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, scratchBuffer.memory, nullptr);
    }
    if (scratchBuffer.handle != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, scratchBuffer.handle, nullptr);
    }
}

bool VulkanRendererPathTracing::isMeshLight(const SceneObject *so)
{
    if (so->has<ComponentMaterial>()) {
        auto material = so->get<ComponentMaterial>().material;
        switch (material->type()) {
            case MaterialType::MATERIAL_PBR_STANDARD: {
                auto pbrStandard = static_cast<VulkanMaterialPBRStandard *>(material);
                if (!isBlack(pbrStandard->emissiveColor(), 0.01F)) {
                    return true;
                }
                break;
            }
            case MaterialType::MATERIAL_LAMBERT: {
                auto lambert = static_cast<VulkanMaterialPBRStandard *>(material);
                if (!isBlack(lambert->emissiveColor(), 0.01F)) {
                    return true;
                }
                break;
            }
            default:
                throw std::runtime_error("VulkanPathTracing::isLight(): Unexpected material");
        }
    }

    return false;
}

void VulkanRendererPathTracing::prepareSceneLights(const Scene &scene, std::vector<LightPT> &sceneLights)
{
    const SceneData &sceneData = scene.getSceneData();

    /* Environment map */
    if (scene.ambientIBLFactor() > 0.0001F && scene.skyboxMaterial() != nullptr) {
        sceneLights.push_back(LightPT());
        sceneLights.back().position.a = 3.F;
    }
}

void VulkanRendererPathTracing::prepareSceneObjectLight(const SceneObject *so,
                                                        uint32_t objectDescriptionIndex,
                                                        const glm::mat4 &t,
                                                        std::vector<LightPT> &sceneLights)
{
    if (so->has<ComponentLight>()) {
        auto cl = so->get<ComponentLight>().light;
        sceneLights.push_back(LightPT());
        if (cl->type() == LightType::POINT_LIGHT) {
            auto l = static_cast<VulkanPointLight *>(cl);
            auto color = l->color();

            sceneLights.back().position = glm::vec4(so->worldPosition(), LightType::POINT_LIGHT);
            sceneLights.back().color = glm::vec4(glm::vec3(color) * color.a, 0.F);
        } else if (cl->type() == LightType::DIRECTIONAL_LIGHT) {
            auto l = static_cast<VulkanDirectionalLight *>(cl);
            auto color = l->color();

            sceneLights.back().position = glm::vec4(0, 0, 0, LightType::DIRECTIONAL_LIGHT);
            sceneLights.back().direction = glm::vec4(so->modelMatrix() * glm::vec4(Transform::WORLD_Z, 0));
            sceneLights.back().color = glm::vec4(glm::vec3(color) * color.a, 0.F);
        }
    }

    if (so->has<ComponentMaterial>() && isMeshLight(so)) {
        auto material = so->get<ComponentMaterial>().material;
        sceneLights.push_back(LightPT());
        sceneLights.back().position = glm::vec4(t[0][0], t[0][1], t[0][2], LightType::MESH_LIGHT);
        sceneLights.back().direction = glm::vec4(t[1][0], t[1][1], t[1][2], objectDescriptionIndex);
        sceneLights.back().color = glm::vec4(t[2][0], t[2][1], t[2][2], 0);
        sceneLights.back().transform = glm::vec4(t[3][0], t[3][1], t[3][2], 0);
    }
}

}  // namespace vengine