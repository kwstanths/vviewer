#include "VulkanScene.hpp"

#include "Console.hpp"

#include "core/AssetManager.hpp"

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/resources/VulkanLight.hpp"

namespace vengine
{

VulkanScene::VulkanScene(VulkanContext &vkctx)
    : m_vkctx(vkctx)
    , m_modelDataUBO(VULKAN_LIMITS_MAX_UNIQUE_TRANSFORMS)
    , m_lightDataUBO(VULKAN_LIMITS_MAX_UNIQUE_LIGHTS)
    , m_lightInstancesUBO(VULKAN_LIMITS_MAX_LIGHT_INSTANCES)
{
}

VulkanScene::~VulkanScene()
{
}

VkResult VulkanScene::initResources()
{
    /* Initialize uniform buffers for model matrix data */
    m_modelDataUBO.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
    /* Initialize uniform buffers for light data */
    m_lightDataUBO.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
    /* Initialize uniform buffers for light instances */
    m_lightInstancesUBO.init(m_vkctx.physicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayouts());

    return VK_SUCCESS;
}

VkResult VulkanScene::initSwapchainResources(uint32_t nImages)
{
    VULKAN_CHECK_CRITICAL(createBuffers(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));
    return VK_SUCCESS;
}

VkResult VulkanScene::releaseResources()
{
    m_modelDataUBO.destroyCPUMemory();
    m_lightDataUBO.destroyCPUMemory();
    m_lightInstancesUBO.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutLight, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutModel, nullptr);
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutScene, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanScene::releaseSwapchainResources()
{
    for (uint32_t i = 0; i < m_uniformBuffersScene.size(); i++) {
        m_uniformBuffersScene[i].destroy(m_vkctx.device());
    }

    m_modelDataUBO.destroyGPUBuffers(m_vkctx.device());
    m_lightDataUBO.destroyGPUBuffers(m_vkctx.device());
    m_lightInstancesUBO.destroyGPUBuffers(m_vkctx.device());

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

void VulkanScene::updateBuffers(const std::vector<SceneObject *> &lights,
                                const std::vector<std::pair<SceneObject *, uint32_t>> &meshLights,
                                uint32_t imageIndex) const
{
    /* TODO check if something has changed */

    /* Update scene data buffer */
    {
        SceneData sceneData = VulkanScene::getSceneData();

        void *data;
        vkMapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory(), 0, sizeof(SceneData), 0, &data);
        memcpy(data, &sceneData, sizeof(sceneData));
        vkUnmapMemory(m_vkctx.device(), m_uniformBuffersScene[imageIndex].memory());
    }

    /* Update transform data to buffer */
    {
        void *data;
        vkMapMemory(m_vkctx.device(),
                    m_modelDataUBO.memory(imageIndex),
                    0,
                    m_modelDataUBO.blockSizeAligned() * m_modelDataUBO.nblocks(),
                    0,
                    &data);
        memcpy(data, m_modelDataUBO.block(0), m_modelDataUBO.blockSizeAligned() * m_modelDataUBO.nblocks());
        vkUnmapMemory(m_vkctx.device(), m_modelDataUBO.memory(imageIndex));
    }

    /* Update light data to buffer */
    {
        void *data;
        vkMapMemory(m_vkctx.device(),
                    m_lightDataUBO.memory(imageIndex),
                    0,
                    m_lightDataUBO.blockSizeAligned() * m_lightDataUBO.nblocks(),
                    0,
                    &data);
        memcpy(data, m_lightDataUBO.block(0), m_lightDataUBO.blockSizeAligned() * m_lightDataUBO.nblocks());
        vkUnmapMemory(m_vkctx.device(), m_lightDataUBO.memory(imageIndex));
    }

    /* Update light instances */
    for (uint32_t l = 0; l < lights.size(); l++) {
        assert(l < m_lightInstancesUBO.nblocks());

        auto vo = static_cast<VulkanSceneObject *>(lights[l]);
        Light *light = vo->get<ComponentLight>().light;

        assert(light != nullptr);

        LightInstance *lightInstanceBlock = m_lightInstancesUBO.block(l);
        lightInstanceBlock->info.r = light->lightIndex();
        lightInstanceBlock->info.g = vo->getModelDataUBOIndex();
        lightInstanceBlock->info.a = static_cast<int32_t>(light->type());

        if (light->type() == LightType::POINT_LIGHT) {
            lightInstanceBlock->position = glm::vec4(vo->worldPosition(), LightType::POINT_LIGHT);
        } else if (light->type() == LightType::DIRECTIONAL_LIGHT) {
            lightInstanceBlock->position = glm::vec4(vo->modelMatrix() * glm::vec4(Transform::WORLD_Z, 0));
        }
    }
    for (uint32_t l = 0; l < meshLights.size(); l++) {
        uint32_t nl = lights.size() + l;
        assert(nl < m_lightInstancesUBO.nblocks());

        auto vo = static_cast<VulkanSceneObject *>(meshLights[l].first);
        auto objectDescriptionIndex = meshLights[l].second;
        Material *material = vo->get<ComponentMaterial>().material;

        assert(material != nullptr);

        LightInstance *lightInstanceBlock = m_lightInstancesUBO.block(nl);
        lightInstanceBlock->info.g = vo->getModelDataUBOIndex();
        lightInstanceBlock->info.b = objectDescriptionIndex;
        lightInstanceBlock->info.a = static_cast<int32_t>(LightType::MESH_LIGHT);

        const auto &m = vo->modelMatrix();
        lightInstanceBlock->position = glm::vec4(m[0][0], m[1][0], m[2][0], m[3][0]);
        lightInstanceBlock->position1 = glm::vec4(m[0][1], m[1][1], m[2][1], m[3][1]);
        lightInstanceBlock->position2 = glm::vec4(m[0][2], m[1][2], m[2][2], m[3][2]);
    }

    /* Update light instances to buffer */
    {
        void *data;
        vkMapMemory(m_vkctx.device(),
                    m_lightInstancesUBO.memory(imageIndex),
                    0,
                    m_lightInstancesUBO.blockSizeAligned() * m_lightInstancesUBO.nblocks(),
                    0,
                    &data);
        memcpy(data, m_lightInstancesUBO.block(0), m_lightInstancesUBO.blockSizeAligned() * m_lightInstancesUBO.nblocks());
        vkUnmapMemory(m_vkctx.device(), m_lightInstancesUBO.memory(imageIndex));
    }
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
    auto object = new VulkanSceneObject(this, name, m_modelDataUBO);
    return object;
}

void VulkanScene::deleteObject(SceneObject *object)
{
    delete object;
}

VkResult VulkanScene::createDescriptorSetsLayouts()
{
    /* Create descriptor layout for the scene data */
    {
        VkDescriptorSetLayoutBinding sceneDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &sceneDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutScene));
    }

    {
        /* Create descriptor layout for the model data */
        VkDescriptorSetLayoutBinding modelDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            0,
            1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &modelDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutModel));
    }

    {
        /* Create descriptor layout for the light data and light instances */
        VkDescriptorSetLayoutBinding lightDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0, 1);

        VkDescriptorSetLayoutBinding lightInstancesLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 1, 1);

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {lightDataLayoutBinding, lightInstancesLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo =
            vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutLight));
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createDescriptorPool(uint32_t nImages)
{
    VkDescriptorPoolSize sceneDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize modelDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightInstancesPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    std::array<VkDescriptorPoolSize, 4> poolSizes = {sceneDataPoolSize, modelDataPoolSize, lightDataPoolSize, lightInstancesPoolSize};

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
        m_descriptorSetsModel.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutModel);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsModel.data()));
    }

    {
        m_descriptorSetsLight.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutLight);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsLight.data()));
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {
        /* SceneData */
        auto bufferInfoScene = vkinit::descriptorBufferInfo(m_uniformBuffersScene[i].buffer(), 0, sizeof(SceneData));
        auto descriptorWriteScene =
            vkinit::writeDescriptorSet(m_descriptorSetsScene[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoScene);

        /* ModelData */
        auto bufferInfoModel = vkinit::descriptorBufferInfo(m_modelDataUBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteModel =
            vkinit::writeDescriptorSet(m_descriptorSetsModel[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoModel);

        /* Lights */
        /* LightData */
        auto bufferInfoLightData = vkinit::descriptorBufferInfo(m_lightDataUBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteLightData =
            vkinit::writeDescriptorSet(m_descriptorSetsLight[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, &bufferInfoLightData);
        /* Light instances */
        auto bufferInfoLightInstances = vkinit::descriptorBufferInfo(m_lightInstancesUBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteLightInstances =
            vkinit::writeDescriptorSet(m_descriptorSetsLight[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, 1, &bufferInfoLightInstances);

        std::array<VkWriteDescriptorSet, 4> writeSets = {
            descriptorWriteScene, descriptorWriteModel, descriptorWriteLightData, descriptorWriteLightInstances};
        vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

VkResult VulkanScene::createBuffers(uint32_t nImages)
{
    VkDeviceSize bufferSize = sizeof(SceneData);

    m_uniformBuffersScene.resize(nImages);
    for (int i = 0; i < nImages; i++) {
        VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                           m_vkctx.device(),
                                           bufferSize,
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           m_uniformBuffersScene[i]));
    }

    m_modelDataUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);
    m_lightDataUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);
    m_lightInstancesUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    return VK_SUCCESS;
}

}  // namespace vengine