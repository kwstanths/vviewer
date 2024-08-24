#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/resources/VulkanLight.hpp"
#include "vulkan/renderers/VulkanRendererPathTracing.hpp"

namespace vengine
{

VulkanScene::VulkanScene(VulkanContext &vkctx)
    : m_vkctx(vkctx)
    , m_instances(vkctx)
    , m_lightDataUBO(VULKAN_LIMITS_MAX_UNIQUE_LIGHTS)
{
}

VulkanScene::~VulkanScene()
{
}

VkResult VulkanScene::initResources()
{
    VULKAN_CHECK_CRITICAL(m_instances.initResources());

    /* Initialize uniform buffers for light data */
    m_lightDataUBO.init(m_vkctx.physicalDeviceProperties());

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayouts());

    return VK_SUCCESS;
}

VkResult VulkanScene::initSwapchainResources(uint32_t nImages)
{
    VULKAN_CHECK_CRITICAL(m_instances.initSwapchainResources(nImages));

    m_tlas.resize(nImages);

    VULKAN_CHECK_CRITICAL(createBuffers(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));
    return VK_SUCCESS;
}

VkResult VulkanScene::releaseResources()
{
    VULKAN_CHECK_CRITICAL(m_instances.releaseResources());

    m_lightDataUBO.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutLight, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutScene, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutTLAS, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanScene::releaseSwapchainResources()
{
    VULKAN_CHECK_CRITICAL(m_instances.releaseSwapchainResources());

    for (uint32_t i = 0; i < m_uniformBuffersScene.size(); i++) {
        m_uniformBuffersScene[i].destroy(m_vkctx.device());
    }
    for (uint32_t i = 0; i < m_tlas.size(); i++) {
        m_tlas[i].destroy(m_vkctx.device());
    }

    m_lightDataUBO.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

SceneData VulkanScene::getSceneData() const
{
    SceneData sceneData = Scene::getSceneData();
    sceneData.m_projection[1][1] *= -1;
    sceneData.m_projectionInverse = glm::inverse(sceneData.m_projection);

    return sceneData;
}

void VulkanScene::update()
{
    Scene::update();

    SceneObjectVector sceneGraphArray = getSceneObjectsFlat();

    m_instances.reset();
    m_instances.build(sceneGraphArray);
}

void VulkanScene::updateFrame(VulkanCommandInfo vci, uint32_t imageIndex)
{
    /* SceneData buffer */
    {
        SceneData sceneData = VulkanScene::getSceneData();

        void *data;
        vkMapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory(), 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory());
    }

    m_instances.updateBuffers(imageIndex);

    m_lightDataUBO.updateBuffer(m_vkctx.device(), imageIndex);

    buildTLAS(vci, imageIndex);
}

Light *VulkanScene::createLight(const AssetInfo &info, LightType type, glm::vec4 color)
{
    auto &lights = AssetManager::getInstance().lightsMap();

    switch (type) {
        case LightType::POINT_LIGHT: {
            auto light = new VulkanPointLight(info, m_lightDataUBO);
            light->color() = color;
            lights.add(light);
            return light;
            break;
        }
        case LightType::DIRECTIONAL_LIGHT: {
            auto light = new VulkanDirectionalLight(info, m_lightDataUBO);
            light->color() = color;
            lights.add(light);
            return light;
            break;
        }
        default: {
            debug_tools::ConsoleWarning("VulkanScene::createLight(): Unknown type of light");
            return nullptr;
        }
    }
}

SceneObject *VulkanScene::createObject(std::string name)
{
    auto object = new VulkanSceneObject(this, name);
    return object;
}

void VulkanScene::deleteObject(SceneObject *object)
{
    delete object;
}

VkResult VulkanScene::createDescriptorSetsLayouts()
{
    /* Create descriptor set layout for the scene data */
    {
        VkDescriptorSetLayoutBinding sceneDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &sceneDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutScene));
    }

    /* Create descriptor set layout for the light data and light instances */
    {
        /* binding 0, LightData matrix */
        VkDescriptorSetLayoutBinding lightDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 1);

        /* binding 0, LightInstance matrix */
        VkDescriptorSetLayoutBinding lightInstancesLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1, 1);

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {lightDataLayoutBinding, lightInstancesLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo =
            vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutLight));
    }

    /* Create descriptor set layout for the TLAS */
    {
        /* binding 0, the accelleration strucure */
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding =
            vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                                               VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                                                   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                                               0,
                                               1);

        std::vector<VkDescriptorSetLayoutBinding> bindings({accelerationStructureLayoutBinding});
        VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo =
            vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
        VULKAN_CHECK_CRITICAL(
            vkCreateDescriptorSetLayout(m_vkctx.device(), &descriptorSetlayoutInfo, nullptr, &m_descriptorSetLayoutTLAS));
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createDescriptorPool(uint32_t nImages)
{
    VkDescriptorPoolSize sceneDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightInstancesPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize TLASPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, nImages);
    VkDescriptorPoolSize objectDescrptionPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nImages);
    std::vector<VkDescriptorPoolSize> poolSizes = {
        sceneDataPoolSize, lightDataPoolSize, lightInstancesPoolSize, TLASPoolSize, objectDescrptionPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),
                                         poolSizes.data(),
                                         static_cast<uint32_t>(2 * nImages + VULKAN_LIMITS_MAX_MATERIALS * nImages));
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanScene::createDescriptorSets(uint32_t nImages)
{
    /* Create descriptor sets */
    {
        m_descriptorSetsScene.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutScene);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsScene.data()));
    }

    {
        m_descriptorSetsLight.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutLight);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsLight.data()));
    }

    {
        m_descriptorSetsTLAS.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutTLAS);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());
        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsTLAS.data()));
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {
        /* SceneData */
        auto bufferInfoScene = vkinit::descriptorBufferInfo(m_uniformBuffersScene[i].buffer(), 0, sizeof(SceneData));
        auto descriptorWriteScene =
            vkinit::writeDescriptorSet(m_descriptorSetsScene[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoScene);

        /* Lights */
        /* LightData */
        auto bufferInfoLightData = vkinit::descriptorBufferInfo(m_lightDataUBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteLightData =
            vkinit::writeDescriptorSet(m_descriptorSetsLight[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoLightData);
        /* Light instances */
        auto bufferInfoLightInstances = vkinit::descriptorBufferInfo(m_instances.m_lightInstancesUBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteLightInstances =
            vkinit::writeDescriptorSet(m_descriptorSetsLight[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, &bufferInfoLightInstances);

        std::vector<VkWriteDescriptorSet> writeSets = {descriptorWriteScene, descriptorWriteLightData, descriptorWriteLightInstances};
        vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createBuffers(uint32_t nImages)
{
    VkDeviceSize bufferSize = sizeof(SceneData);

    m_uniformBuffersScene.resize(nImages);
    for (uint32_t i = 0; i < nImages; i++) {
        VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                           m_vkctx.device(),
                                           bufferSize,
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           m_uniformBuffersScene[i]));
    }

    m_lightDataUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    return VK_SUCCESS;
}

void VulkanScene::buildTLAS(VulkanCommandInfo vci, uint32_t imageIndex)
{
    /* TODO update TLAS don't recreate */

    /* Create new TLAS */
    std::vector<BLASInstance> blasInstances;
    const VulkanInstancesManager &instances = instancesManager();
    for (auto &meshGroup : instances.opaqueMeshes()) {
        uint32_t nObjects = static_cast<uint32_t>(meshGroup.second.sceneObjects.size());
        for (uint32_t index = 0; index < nObjects; ++index) {
            SceneObject *sceneObject = meshGroup.second.sceneObjects[index];

            auto mesh = static_cast<VulkanMesh *>(sceneObject->get<ComponentMesh>().mesh);
            assert(mesh->blas().initialized());

            auto material = sceneObject->get<ComponentMaterial>().material;
            MaterialType matType = material->type();
            uint32_t instanceSBTOffset = VulkanRendererPathTracing::SBTMaterialOffset(matType);

            blasInstances.emplace_back(
                mesh->blas(), sceneObject->modelMatrix(), instanceSBTOffset, m_instances.instanceDataIndex(sceneObject));
        }
    }
    for (SceneObject *so : instances.transparentMeshes()) {
        Material *mat = so->get<ComponentMaterial>().material;
        assert(mat != nullptr);

        uint32_t instanceSBTOffset = VulkanRendererPathTracing::SBTMaterialOffset(mat->type());

        auto mesh = static_cast<VulkanMesh *>(so->get<ComponentMesh>().mesh);
        assert(mesh->blas().initialized());
        blasInstances.emplace_back(mesh->blas(), so->modelMatrix(), instanceSBTOffset, m_instances.instanceDataIndex(so));
    }
    for (SceneObject *so : instances.volumes()) {
        Material *mat = so->get<ComponentMaterial>().material;
        assert(mat != nullptr);

        uint32_t instanceSBTOffset = VulkanRendererPathTracing::SBTMaterialOffset(mat->type());

        auto mesh = static_cast<VulkanMesh *>(so->get<ComponentMesh>().mesh);
        assert(mesh->blas().initialized());
        blasInstances.emplace_back(mesh->blas(), so->modelMatrix(), instanceSBTOffset, m_instances.instanceDataIndex(so));
    }

    m_tlas[imageIndex].destroy(m_vkctx.device());
    createTopLevelAccelerationStructure(blasInstances, vci, imageIndex);

    /* Update accelleration structure descriptor */
    VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
    descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
    descriptorAccelerationStructureInfo.pAccelerationStructures = &m_tlas[imageIndex].handle();
    VkWriteDescriptorSet accelerationStructureWrite{};
    accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    /* The specialized acceleration structure descriptor has to be chained */
    accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
    accelerationStructureWrite.dstSet = m_descriptorSetsTLAS[imageIndex];
    accelerationStructureWrite.dstBinding = 0;
    accelerationStructureWrite.descriptorCount = 1;
    accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    vkUpdateDescriptorSets(m_vkctx.device(), 1, &accelerationStructureWrite, 0, nullptr);
}

void VulkanScene::createTopLevelAccelerationStructure(const std::vector<BLASInstance> &blasInstances,
                                                      VulkanCommandInfo vci,
                                                      uint32_t imageIndex)
{
    /* Create a buffer to hold top level instances */
    std::vector<VkAccelerationStructureInstanceKHR> instances(blasInstances.size());
    for (size_t i = 0; i < blasInstances.size(); i++) {
        auto &t = blasInstances[i].modelMatrix;
        /* Set transform matrix per blas */
        VkTransformMatrixKHR transformMatrix = {
            t[0][0], t[1][0], t[2][0], t[3][0], t[0][1], t[1][1], t[2][1], t[3][1], t[0][2], t[1][2], t[2][2], t[3][2]};

        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = blasInstances[i].instanceDataOffset; /* Index to InstanceData buffer */
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = blasInstances[i].SBTOffset; /* SBT offset */
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = blasInstances[i].accelerationStructure.buffer().address().deviceAddress;
    }

    m_tlas[imageIndex].initializeTopLevelAcceslerationStructure({vci.physicalDevice, vci.device, vci.commandPool, vci.queue},
                                                                instances);
}

}  // namespace vengine