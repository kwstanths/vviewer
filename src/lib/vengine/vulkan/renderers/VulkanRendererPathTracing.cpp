#include "VulkanRendererPathTracing.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
#include <sys/types.h>
#include <vector>

#include <OpenImageDenoise/oidn.hpp>

#include <debug_tools/Console.hpp>
#include <debug_tools/Timer.hpp>

#include "core/Materials.hpp"
#include "utils/ECS.hpp"
#include "core/ImageUtils.hpp"
#include "vulkan/common/IncludeVulkan.hpp"
#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/common/VulkanUtils.hpp"
#include "vulkan/common/VulkanShader.hpp"
#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/common/VulkanDeviceFunctions.hpp"
#include "vulkan/resources/VulkanLight.hpp"

namespace vengine
{

VulkanRendererPathTracing::VulkanRendererPathTracing(VulkanContext &vkctx,
                                                     VulkanScene &scene,
                                                     VulkanMaterials &materials,
                                                     VulkanTextures &textures,
                                                     VulkanRandom &random)
    : m_vkctx(vkctx)
    , m_scene(scene)
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
    if (m_format != VK_FORMAT_R32G32B32A32_SFLOAT) {
        std::string error = "VulkanPathTracing::initResources(): Format supported is VK_FORMAT_R32G32B32A32_SFLOAT";
        debug_tools::ConsoleCritical(error);
        throw std::runtime_error(error);
    }

    m_descriptorSetLayoutSkybox = skyboxDescriptorLayout;

    m_physicalDeviceProperties = m_vkctx.physicalDeviceProperties();

    m_device = m_vkctx.device();
    m_commandPool = m_vkctx.graphicsCommandPool();
    m_queue = m_vkctx.queueManager().graphicsQueue();

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

    m_tempImage.destroy(m_device);
    m_renderResultRadiance.destroy(m_device);
    m_renderResultAlbedo.destroy(m_device);
    m_renderResultNormal.destroy(m_device);

    return VK_SUCCESS;
}

bool VulkanRendererPathTracing::isRayTracingEnabled() const
{
    return m_isInitialized;
}

void VulkanRendererPathTracing::render()
{
    if (m_renderInProgress)
        return;

    m_renderProgress = 0.0F;

    debug_tools::Timer timer;
    timer.Start();

    /* Get the scene objects */
    SceneObjectVector sceneObjects = m_scene.getSceneObjectsFlat();
    if (sceneObjects.size() == 0) {
        debug_tools::ConsoleWarning("Trying to render an empty scene");
        return;
    }

    m_renderInProgress = true;

    auto camera = m_scene.camera();

    /* Prepare sceneData with the custom render resolution requested */
    SceneData sceneData = m_scene.getSceneData();
    {
        uint32_t width = renderInfo().width;
        uint32_t height = renderInfo().height;

        /* Change to requested render resolution */
        switch (camera->type()) {
            case CameraType::PERSPECTIVE: {
                auto perspectiveCamera = std::static_pointer_cast<PerspectiveCamera>(camera);
                sceneData.m_projection = glm::perspective(
                    glm::radians(perspectiveCamera->fov()), static_cast<float>(width) / height, camera->znear(), camera->zfar());
                sceneData.m_projection[1][1] *= -1;
                sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);
                break;
            }
            case CameraType::ORTHOGRAPHIC: {
                auto orthoCamera = std::static_pointer_cast<OrthographicCamera>(camera);
                auto owidth = static_cast<float>(width);
                auto oheight = static_cast<float>(height);
                sceneData.m_projection =
                    glm::ortho(-owidth / 2, owidth / 2, -oheight / 2, oheight / 2, camera->znear(), camera->zfar());
                sceneData.m_projection[1][1] *= -1;
                sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);
            }
            default:
                break;
        }
    }

    /* Set camera volume */
    sceneData.m_volumes.r = -1;
    if (camera->volume() == nullptr) {
        /* Check if camera is inside a volume AABB, but a camera volume is not set and give a warning */
        glm::vec3 cameraPosition = camera->transform().position();
        const SceneObjectVector &volumes = m_scene.instancesManager().volumes();
        for (SceneObject *so : volumes) {
            if (so->get<ComponentMaterial>().material()->type() == MaterialType::MATERIAL_VOLUME) {
                if (so->AABB().isInside(cameraPosition)) {
                    debug_tools::ConsoleWarning(
                        "The render camera has no volume set, but it is inside a volume AABB. Forgot to set camera volume "
                        "material?");
                }
            }
        }
    } else {
        Material *cameraVolume = camera->volume();
        if (cameraVolume->type() != MaterialType::MATERIAL_VOLUME) {
            debug_tools::ConsoleWarning("The material set for the render camera's volume is not a volume material");
        } else {
            sceneData.m_volumes.r = static_cast<float>(cameraVolume->materialIndex());
        }
    }

    /* Prepare scene lights */
    m_pathTracingData.lights.r =
        static_cast<unsigned int>(m_scene.instancesManager().lights().size() + m_scene.instancesManager().meshLights().size());
    updateBuffers(sceneData, m_pathTracingData);

    /* We will always use frame index 0 buffers for this renderer */
    m_scene.updateFrame(
        {m_vkctx.physicalDevice(), m_vkctx.device(), m_vkctx.graphicsCommandPool(), m_vkctx.queueManager().graphicsQueue()}, 0);
    m_materials.updateBuffers(0);
    m_textures.updateTextures();

    /* Get skybox descriptor */
    auto skybox = dynamic_cast<const VulkanMaterialSkybox *>(m_scene.skyboxMaterial());
    assert(skybox != nullptr);
    if (skybox->needsUpdate(0)) {
        skybox->updateDescriptorSet(m_device, 0);
    }

    setResolution();
    auto res = render(skybox->getDescriptor(0));

    timer.Stop();

    if (res == VK_SUCCESS) {
        std::string fileExtension = (renderInfo().fileType == FileType::PNG) ? "png" : "hdr";
        std::string filename = renderInfo().filename + "." + fileExtension;
        debug_tools::ConsoleInfo("Scene rendered: " + filename + " in: " + std::to_string(timer.ToInt()) + "ms");
    }

    m_renderInProgress = false;
}

float VulkanRendererPathTracing::renderProgress()
{
    return m_renderProgress;
}

uint32_t VulkanRendererPathTracing::SBTMaterialOffset(MaterialType type)
{
    static const uint32_t nTypeRays = 3U;

    if (type == MaterialType::MATERIAL_LAMBERT)
        return 0 * nTypeRays;
    else if (type == MaterialType::MATERIAL_PBR_STANDARD)
        return 1 * nTypeRays;
    else if (type == MaterialType::MATERIAL_VOLUME) {
        return 2 * nTypeRays;
    } else {
        throw std::runtime_error("VulkanRendererPathTracing::SBTMaterialOffset(): Unexpected material");
    }
}

void VulkanRendererPathTracing::setResolution()
{
    m_renderResultRadiance.destroy(m_device);
    m_renderResultAlbedo.destroy(m_device);
    m_renderResultNormal.destroy(m_device);
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

VkResult VulkanRendererPathTracing::createStorageImage(uint32_t width, uint32_t height)
{
    /* Create images for the render targets used during path tracing */
    /* Radiance target */
    {
        VkImageCreateInfo imageInfo =
            vkinit::imageCreateInfo({width, height, 1},
                                    m_format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                          m_device,
                                          imageInfo,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          m_renderResultRadiance.image(),
                                          m_renderResultRadiance.memory()));

        VkImageViewCreateInfo imageInfoView =
            vkinit::imageViewCreateInfo(m_renderResultRadiance.image(), m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &imageInfoView, nullptr, &m_renderResultRadiance.view()));
    }

    /* Albedo target */
    {
        VkImageCreateInfo imageInfo =
            vkinit::imageCreateInfo({width, height, 1},
                                    m_format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                          m_device,
                                          imageInfo,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          m_renderResultAlbedo.image(),
                                          m_renderResultAlbedo.memory()));

        VkImageViewCreateInfo imageInfoView =
            vkinit::imageViewCreateInfo(m_renderResultAlbedo.image(), m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &imageInfoView, nullptr, &m_renderResultAlbedo.view()));
    }

    /* Normal target */
    {
        VkImageCreateInfo imageInfo =
            vkinit::imageCreateInfo({width, height, 1},
                                    m_format,
                                    1,
                                    VK_SAMPLE_COUNT_1_BIT,
                                    VK_IMAGE_TILING_OPTIMAL,
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                          m_device,
                                          imageInfo,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                          m_renderResultNormal.image(),
                                          m_renderResultNormal.memory()));

        VkImageViewCreateInfo imageInfoView =
            vkinit::imageViewCreateInfo(m_renderResultNormal.image(), m_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        VULKAN_CHECK_CRITICAL(vkCreateImageView(m_device, &imageInfoView, nullptr, &m_renderResultNormal.view()));
    }

    /* Create temp image used to copy render result from gpu memory to cpu memory */
    {
        VkImageCreateInfo imageInfo = vkinit::imageCreateInfo(
            {width, height, 1}, m_format, 1, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        VULKAN_CHECK_CRITICAL(createImage(m_vkctx.physicalDevice(),
                                          m_device,
                                          imageInfo,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                          m_tempImage.image(),
                                          m_tempImage.memory()));
    }

    /* Transition images to the approriate layout ready for render */
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_device, m_commandPool, cmdBuf));

    transitionImageLayout(cmdBuf,
                          m_renderResultRadiance.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_GENERAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    transitionImageLayout(cmdBuf,
                          m_renderResultAlbedo.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_GENERAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    transitionImageLayout(cmdBuf,
                          m_renderResultNormal.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_GENERAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    transitionImageLayout(cmdBuf,
                          m_tempImage.image(),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf));

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createBuffers()
{
    /* The scene buffer holds [SceneData | PathTracingData ] */
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    uint32_t totalSceneBufferSize = alignedSize(sizeof(SceneData), static_cast<uint32_t>(alignment)) +
                                    alignedSize(sizeof(PathTracingData), static_cast<uint32_t>(alignment));

    /* Create a buffer to hold the scene buffer */
    VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                       m_device,
                                       totalSceneBufferSize,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       m_uniformBufferScene));

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::updateBuffers(const SceneData &sceneData, const PathTracingData &rtData)
{
    /* Update scene buffer */
    {
        VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_uniformBufferScene.memory(), 0, VK_WHOLE_SIZE, 0, &data));
        memcpy(data, &sceneData, sizeof(sceneData)); /* Copy scene data struct */
        memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), static_cast<uint32_t>(alignment)),
               &rtData,
               sizeof(PathTracingData)); /* Copy path tracing data struct */
        vkUnmapMemory(m_device, m_uniformBufferScene.memory());
    }

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::updateBuffersPathTracingData(const PathTracingData &ptData)
{
    VkDeviceSize alignment = m_physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

    void *data;
    VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_uniformBufferScene.memory(), 0, VK_WHOLE_SIZE, 0, &data));
    memcpy(static_cast<char *>(data) + alignedSize(sizeof(SceneData), static_cast<uint32_t>(alignment)),
           &ptData,
           sizeof(PathTracingData)); /* Copy path tracing data struct */

    vkUnmapMemory(m_device, m_uniformBufferScene.memory());

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::createDescriptorSets()
{
    /* Main set is:
     * 3 storages image for output
     * 1 uniform buffer for the scene data
     */
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
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

    /* Update render target storage images binding */
    VkDescriptorImageInfo storageImageDescriptor[3];
    storageImageDescriptor[0] = vkinit::descriptorImageInfo(VK_NULL_HANDLE, m_renderResultRadiance.view(), VK_IMAGE_LAYOUT_GENERAL);
    storageImageDescriptor[1] = vkinit::descriptorImageInfo(VK_NULL_HANDLE, m_renderResultAlbedo.view(), VK_IMAGE_LAYOUT_GENERAL);
    storageImageDescriptor[2] = vkinit::descriptorImageInfo(VK_NULL_HANDLE, m_renderResultNormal.view(), VK_IMAGE_LAYOUT_GENERAL);
    VkWriteDescriptorSet resultImageWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, 3, storageImageDescriptor);

    /* Update scene data binding */
    VkDescriptorBufferInfo sceneDataDescriptor = vkinit::descriptorBufferInfo(m_uniformBufferScene.buffer(), 0, sizeof(SceneData));
    VkWriteDescriptorSet sceneDataWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, &sceneDataDescriptor);

    /* Update path tracing data binding, uses the same buffer with the scene data */
    VkDescriptorBufferInfo pathTracingDataDescriptor =
        vkinit::descriptorBufferInfo(m_uniformBufferScene.buffer(),
                                     alignedSize(sizeof(SceneData), static_cast<uint32_t>(uniformBufferAlignment)),
                                     static_cast<uint32_t>(sizeof(PathTracingData)));
    VkWriteDescriptorSet pathTracingDataWrite =
        vkinit::writeDescriptorSet(m_descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, 1, &pathTracingDataDescriptor);

    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {resultImageWrite, sceneDataWrite, pathTracingDataWrite};
    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

VkResult VulkanRendererPathTracing::createRayTracingPipeline()
{
    /* Create the main descriptor set */
    {
        /* Binding 0, the output images */
        VkDescriptorSetLayoutBinding resultImageLayoutBinding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, 3);
        /* Binding 1, the scene data */
        VkDescriptorSetLayoutBinding sceneDataBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
            1,
            1);
        /* Binding 2, the path tracing data */
        VkDescriptorSetLayoutBinding pathTracingDataBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
            2,
            1);

        std::vector<VkDescriptorSetLayoutBinding> bindings(
            {resultImageLayoutBinding, sceneDataBufferBinding, pathTracingDataBufferBinding});
        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo =
            vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_device, &descriptorSetlayoutInfo, nullptr, &m_descriptorSetLayoutMain));
    }

    /* Create pipeline layout */
    {
        std::vector<VkDescriptorSetLayout> setsLayouts = {m_descriptorSetLayoutMain,
                                                          m_scene.descriptorSetlayoutInstanceData(),
                                                          m_scene.descriptorSetlayoutTLAS(),
                                                          m_materials.descriptorSetLayout(),
                                                          m_textures.descriptorSetLayout(),
                                                          m_scene.descriptorSetlayoutLights(),
                                                          m_descriptorSetLayoutSkybox,
                                                          m_random.descriptorSetLayout()};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo =
            vkinit::pipelineLayoutCreateInfo(static_cast<uint32_t>(setsLayouts.size()), setsLayouts.data(), 0, nullptr);
        VULKAN_CHECK_CRITICAL(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));
    }

    /* --- PATH TRACING SHADERS --- */

    /*
        Setup shader groups
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

    /* Any hit shaders */
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

    /* Secondary ray volume any hit shader */
    VkPipelineShaderStageCreateInfo raySecondaryVolumeAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitSecondaryVolume.rahit.spv"), "main");
    shaderStages.push_back(raySecondaryVolumeAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitSecondaryVolume = static_cast<uint32_t>(shaderStages.size()) - 1;

    /* NEE any hit shader */
    VkPipelineShaderStageCreateInfo rayNEEAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitNEE.rahit.spv"), "main");
    shaderStages.push_back(rayNEEAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitNEE = static_cast<uint32_t>(shaderStages.size()) - 1;

    /* NEE volume any hit shader */
    VkPipelineShaderStageCreateInfo rayNEEVolumeAnyHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/rayahitNEEVolume.rahit.spv"), "main");
    shaderStages.push_back(rayNEEVolumeAnyHitShaderStageInfo);
    uint32_t shaderStageAnyHitNEEVolume = static_cast<uint32_t>(shaderStages.size()) - 1;

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

    /* Hit group, volume material, Primary ray */
    {
        VkPipelineShaderStageCreateInfo rayClosestHitShaderStageInfo = vkinit::pipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VulkanShader::load(m_device, "shaders/SPIRV/pt/raychitVolume.rchit.spv"), "main");
        shaderStages.push_back(rayClosestHitShaderStageInfo);
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ static_cast<uint32_t>(shaderStages.size()) - 1,
                                                       /* anyHitShader = */ VK_SHADER_UNUSED_KHR);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, volume material, Secondary ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitSecondaryVolume);
        m_shaderGroups.push_back(shaderGroup);
    }
    /* Hit group, volume material, NEE ray */
    {
        VkRayTracingShaderGroupCreateInfoKHR shaderGroup =
            vkinit::rayTracingShaderGroupCreateInfoKHR(VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
                                                       /* generalShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* closestHitShader = */ VK_SHADER_UNUSED_KHR,
                                                       /* anyHitShader = */ shaderStageAnyHitNEEVolume);
        m_shaderGroups.push_back(shaderGroup);
    }

    /* Spec only guarantees 1 level of "recursion". Check for that sad possibility here */
    if (m_rayTracingPipelineProperties.maxRayRecursionDepth <= 1) {
        throw std::runtime_error("VulkanRendererPathTracing::createRayTracingPipeline(): Device doesn't support ray recursion");
    }

    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

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
    VULKAN_CHECK_CRITICAL(devF->vkCreateRayTracingPipelinesKHR(
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

    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

    std::vector<uint8_t> shaderHandleStorage(sbtSize);
    VkResult res =
        devF->vkGetRayTracingShaderGroupHandlesKHR(m_device, m_pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data());
    if (res != VK_SUCCESS) {
        throw std::runtime_error("VulkanRendererPathTracing::createShaderBindingTable(): Failed to get the shader group handles: " +
                                 std::to_string(res));
    }

    /* Create shader binding tables for each group, ray gen, miss and chit, and store the shader group handles */
    const VkBufferUsageFlags bufferUsageFlags =
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    uint32_t nGenGroups = 1U;
    uint32_t nMissGroups = 3U;
    uint32_t nHitGroups = 9U;
    VULKAN_CHECK_CRITICAL(createBuffer(
        m_vkctx.physicalDevice(), m_device, nGenGroups * handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raygenBuffer));
    VULKAN_CHECK_CRITICAL(createBuffer(
        m_vkctx.physicalDevice(), m_device, nMissGroups * handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raymissBuffer));
    VULKAN_CHECK_CRITICAL(createBuffer(
        m_vkctx.physicalDevice(), m_device, nHitGroups * handleSize, bufferUsageFlags, memoryUsageFlags, m_sbt.raychitBuffer));

    /* gen group */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raygenBuffer.memory(), 0, nGenGroups * handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data(), nGenGroups * handleSize);
        vkUnmapMemory(m_device, m_sbt.raygenBuffer.memory());
    }
    /* miss groups */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raymissBuffer.memory(), 0, nMissGroups * handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data() + nGenGroups * handleSizeAligned, nMissGroups * handleSize);
        vkUnmapMemory(m_device, m_sbt.raymissBuffer.memory());
    }
    /* hit groups */
    {
        void *data;
        VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_sbt.raychitBuffer.memory(), 0, nHitGroups * handleSize, 0, &data));
        memcpy(data, shaderHandleStorage.data() + (nGenGroups + nMissGroups) * handleSizeAligned, nHitGroups * handleSize);
        vkUnmapMemory(m_device, m_sbt.raychitBuffer.memory());
    }

    /* Get strided device addresses for the shader binding tables where the shader group handles are stored */
    m_sbt.raygenSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raygenBuffer.buffer()).deviceAddress;
    m_sbt.raygenSbtEntry.stride = handleSizeAligned;
    m_sbt.raygenSbtEntry.size = handleSizeAligned;
    m_sbt.raymissSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raymissBuffer.buffer()).deviceAddress;
    m_sbt.raymissSbtEntry.stride = handleSizeAligned;
    m_sbt.raymissSbtEntry.size = nMissGroups * handleSizeAligned;
    m_sbt.raychitSbtEntry.deviceAddress = getBufferDeviceAddress(m_device, m_sbt.raychitBuffer.buffer()).deviceAddress;
    m_sbt.raychitSbtEntry.stride = handleSizeAligned;
    m_sbt.raychitSbtEntry.size = nHitGroups * handleSizeAligned;
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

    auto devF = VulkanDeviceFunctions::getInstance().rayTracingPipeline();

    updateDescriptorSets();

    debug_tools::ConsoleInfo("Launching render with: " + std::to_string(batches * batchSize) + " samples");
    VkResult res = VK_SUCCESS;
    /* TODO some calls can go outside the loop? */
    for (uint32_t batch = 0; batch < batches; batch+=1) {
        VULKAN_CHECK_CRITICAL(vkBeginCommandBuffer(commandBuffer, &beginInfo));

        PathTracingData batchData = m_pathTracingData;
        batchData.samplesBatchesDepthIndex.r = batchSize;
        batchData.samplesBatchesDepthIndex.g = batches;
        batchData.samplesBatchesDepthIndex.a = batch;
        VULKAN_CHECK_CRITICAL(updateBuffersPathTracingData(batchData));

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_pipeline);

        std::vector<VkDescriptorSet> descriptorSets = {m_descriptorSet,                      /* 0 */
                                                       m_scene.descriptorSetInstanceData(0), /* 1 */
                                                       m_scene.descriptorSetTLAS(0),         /* 2 */
                                                       m_materials.descriptorSet(0),         /* 3 */
                                                       m_textures.descriptorSet(),           /* 4 */
                                                       m_scene.descriptorSetLight(0),        /* 5 */
                                                       skyboxDescriptor,                     /* 6 */
                                                       m_random.descriptorSet()};            /* 7 */
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
        devF->vkCmdTraceRaysKHR(commandBuffer,
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
        res = vkWaitForFences(m_device, 1, &fence, VK_TRUE, VULKAN_TIMEOUT_1S);
        if (res == VK_SUCCESS) {
            vkDestroyFence(m_device, fence, nullptr);
        } else if (res == VK_TIMEOUT) {
            debug_tools::ConsoleCritical(
                "VulkanRendererPathTracing::render(): Render failed with timeout. Try again with smaller batch size");
            break;
        } else if (res == VK_ERROR_DEVICE_LOST) {
            debug_tools::ConsoleCritical("VulkanRendererPathTracing::render(): Render failed with Device Lost");
            break;
        } else {
            debug_tools::ConsoleCritical("VulkanRendererPathTracing::render(): Render failed: " + std::to_string(res));
            break;
        }

        m_renderProgress = static_cast<float>(batch) / static_cast<float>(batches);
    }

    if (res == VK_SUCCESS) {
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);

        std::vector<float> radiance, albedo, normal;
        VULKAN_CHECK_CRITICAL(getRenderTargetData(m_renderResultRadiance, radiance));
        VULKAN_CHECK_CRITICAL(getRenderTargetData(m_renderResultAlbedo, albedo));
        VULKAN_CHECK_CRITICAL(getRenderTargetData(m_renderResultNormal, normal));

        VULKAN_CHECK_CRITICAL(storeToDisk(radiance, albedo, normal));
    }

    VULKAN_CHECK_CRITICAL(vkQueueWaitIdle(m_queue));

    return res;
}

VkResult VulkanRendererPathTracing::getRenderTargetData(const VulkanImage &target, std::vector<float> &data)
{
    VkCommandBuffer cmdBuf;
    VULKAN_CHECK_CRITICAL(beginSingleTimeCommands(m_device, m_commandPool, cmdBuf));

    VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    /* Transition render target to transfer source optimal */
    transitionImageLayout(cmdBuf, target.image(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    /* Copy render results to temp image */
    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {renderInfo().width, renderInfo().height, 1};
    vkCmdCopyImage(cmdBuf,
                   target.image(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_tempImage.image(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &copyRegion);

    /* Transition render target back to layout general */
    transitionImageLayout(cmdBuf, target.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange);

    VULKAN_CHECK_CRITICAL(endSingleTimeCommands(m_device, m_commandPool, m_queue, cmdBuf, true, VULKAN_TIMEOUT_100S));

    /* Copy the data from tempImage to data */
    float *input;
    VULKAN_CHECK_CRITICAL(vkMapMemory(m_device, m_tempImage.memory(), 0, VK_WHOLE_SIZE, 0, reinterpret_cast<void **>(&input)));

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
    VkSubresourceLayout subResourceLayout;
    vkGetImageSubresourceLayout(m_device, m_tempImage.image(), &subResource, &subResourceLayout);
    uint32_t rowPitchFloats = static_cast<uint32_t>(subResourceLayout.rowPitch) / sizeof(float);

    uint32_t width = renderInfo().width;
    uint32_t height = renderInfo().height;
    data = std::vector<float>(width * height * 4);

    uint32_t indexGPUIMage = static_cast<uint32_t>(subResourceLayout.offset);
    uint32_t indexOutputImage = 0;
    for (uint32_t i = 0; i < height; i++) {
        memcpy(&data[0] + indexOutputImage, &input[0] + indexGPUIMage, 4 * width * sizeof(float));
        indexGPUIMage += rowPitchFloats;
        indexOutputImage += 4 * width;
    }
    vkUnmapMemory(m_device, m_tempImage.memory());

    return VK_SUCCESS;
}

VkResult VulkanRendererPathTracing::storeToDisk(std::vector<float> &radiance,
                                                std::vector<float> &albedo,
                                                std::vector<float> &normal) const
{
    uint32_t width = renderInfo().width;
    uint32_t height = renderInfo().height;
    uint32_t channels = 4;

    if (renderInfo().writeAllFiles) {
        writeToDisk(albedo, renderInfo().filename + "_albedo", FileType::HDR, width, height, channels);
        writeToDisk(normal, renderInfo().filename + "_normal", FileType::HDR, width, height, channels);
    }

    if (!renderInfo().denoise) {
        if (renderInfo().fileType == FileType::PNG && renderInfo().exposure != 0.0F) {
            applyExposure(radiance, renderInfo().exposure, channels);
        }

        writeToDisk(radiance, renderInfo().filename, renderInfo().fileType, width, height, channels);
        return VK_SUCCESS;
    }

    /* write intermediate radiance as well */
    if (renderInfo().writeAllFiles) {
        writeToDisk(radiance, renderInfo().filename + "_radiance", FileType::HDR, width, height, channels);
    }

    /* Denoise */
    {
        int n_threads = 10;
        bool pin_threads = true;

        oidn::DeviceRef device = oidn::newDevice();
        device.set("numdsThreads", n_threads);
        device.set("setAffinity", pin_threads);
        device.commit();

        oidn::FilterRef filter =
            device.newFilter("RT"); /* generic ray tracing filter, recommened for images generated with Monte Carlo */

        /* Transform normals to [-1, 1] range */
        for (auto &nc : normal) {
            nc = nc * 2.0F - 1.F;
        }

        filter.setImage(
            "color", radiance.data(), oidn::Format::Float3, width, height, 0, 4 * sizeof(float), width * 4 * sizeof(float));
        filter.setImage("albedo", albedo.data(), oidn::Format::Float3, width, height, 0, 4 * sizeof(float), width * 4 * sizeof(float));
        filter.setImage("normal", normal.data(), oidn::Format::Float3, width, height, 0, 4 * sizeof(float), width * 4 * sizeof(float));
        filter.setImage(
            "output", radiance.data(), oidn::Format::Float3, width, height, 0, 4 * sizeof(float), width * 4 * sizeof(float));
        filter.commit();

        filter.execute();

        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None) {
            debug_tools::ConsoleWarning("VulkanRendererPathTracing::storeToDisk(): OpenImageDenoise error [" + std::string(errorMessage) +
                                        "]");
        }
    }

    if (renderInfo().fileType == FileType::PNG && renderInfo().exposure != 0.0F) {
        applyExposure(radiance, renderInfo().exposure, 4);
    }

    writeToDisk(radiance, renderInfo().filename, renderInfo().fileType, width, height, 4);

    return VK_SUCCESS;
}

}  // namespace vengine
