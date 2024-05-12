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
    m_tlas.resize(nImages);

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
    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutTLAS, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanScene::releaseSwapchainResources()
{
    for (uint32_t i = 0; i < m_uniformBuffersScene.size(); i++) {
        m_uniformBuffersScene[i].destroy(m_vkctx.device());
    }
    for (uint32_t i = 0; i < m_storageBufferObjectDescription.size(); i++) {
        m_storageBufferObjectDescription[i].destroy(m_vkctx.device());
    }
    for (uint32_t i = 0; i < m_tlas.size(); i++) {
        m_tlas[i].destroy(m_vkctx.device());
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

void VulkanScene::updateBuffers(const std::vector<SceneObject *> &meshes,
                                const std::vector<SceneObject *> &lights,
                                const std::vector<std::pair<SceneObject *, uint32_t>> &meshLights,
                                uint32_t imageIndex)
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
        lightInstanceBlock->position.a = static_cast<float>(vo->get<ComponentLight>().castShadows);
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

void VulkanScene::updateTLAS(VulkanCommandInfo vci, const std::vector<SceneObject *> &meshes, uint32_t imageIndex)
{
    /* TODO check if something has changed */
    /* TODO update TLAS don't recreate */

    /* Create new TLAS */
    m_blasInstances.clear();
    m_sceneObjectsDescription.clear();
    for (size_t i = 0; i < meshes.size(); i++) {
        auto so = meshes[i];

        auto mesh = static_cast<VulkanMesh *>(so->get<ComponentMesh>().mesh);
        auto material = so->get<ComponentMaterial>().material;

        uint32_t nTypeRays = 3U; /* Types of rays, Primary, Secondary(shadow), NEE */
        uint32_t instanceOffset; /* Ioffset in SBT is based on the type of material */
        if (material->type() == MaterialType::MATERIAL_LAMBERT)
            instanceOffset = 0 * nTypeRays;
        else if (material->type() == MaterialType::MATERIAL_PBR_STANDARD)
            instanceOffset = 1 * nTypeRays;
        else if (material->type() == MaterialType::MATERIAL_VOLUME) {
            instanceOffset = 2 * nTypeRays;
        } else {
            throw std::runtime_error("VulkanScene::updateTLAS(): Unexpected material");
        }

        assert(mesh->blas().initialized());
        m_blasInstances.emplace_back(mesh->blas(), so->modelMatrix(), instanceOffset);
        m_sceneObjectsDescription.push_back({mesh->vertexBuffer().address().deviceAddress,
                                             mesh->indexBuffer().address().deviceAddress,
                                             material->materialIndex(),
                                             mesh->nTriangles()});
    }

    m_tlas[imageIndex].destroy(m_vkctx.device());
    createTopLevelAccelerationStructure(vci, imageIndex);

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

    /* Update buffer with meshes description */
    if (m_sceneObjectsDescription.size() > VULKAN_LIMITS_MAX_OBJECTS) {
        throw std::runtime_error("VulkaScene::updateTLAS(): Number of objects exceeded");
    }
    {
        {
            void *data;
            vkMapMemory(m_vkctx.device(), m_storageBufferObjectDescription[imageIndex].memory(), 0, VK_WHOLE_SIZE, 0, &data);
            memcpy(data, m_sceneObjectsDescription.data(), m_sceneObjectsDescription.size() * sizeof(ObjectDescriptionPT));
            vkUnmapMemory(m_vkctx.device(), m_storageBufferObjectDescription[imageIndex].memory());
        }
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

    /* Create descriptor set layout for the model data */
    {
        VkDescriptorSetLayoutBinding modelDataLayoutBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            0,
            1);

        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::descriptorSetLayoutCreateInfo(1, &modelDataLayoutBinding);
        VULKAN_CHECK_CRITICAL(vkCreateDescriptorSetLayout(m_vkctx.device(), &layoutInfo, nullptr, &m_descriptorSetLayoutModel));
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

        /* Binding 1, the object descriptions buffer */
        VkDescriptorSetLayoutBinding objectDescrptionBufferBinding = vkinit::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, 1, 1);

        std::vector<VkDescriptorSetLayoutBinding> bindings({accelerationStructureLayoutBinding, objectDescrptionBufferBinding});
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
    VkDescriptorPoolSize modelDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize lightInstancesPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nImages);
    VkDescriptorPoolSize TLASPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, nImages);
    VkDescriptorPoolSize objectDescrptionPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nImages);
    std::vector<VkDescriptorPoolSize> poolSizes = {
        sceneDataPoolSize, modelDataPoolSize, lightDataPoolSize, lightInstancesPoolSize, TLASPoolSize, objectDescrptionPoolSize};

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

        /* Object description */
        VkDescriptorBufferInfo objectDescriptionDataDesriptor =
            vkinit::descriptorBufferInfo(m_storageBufferObjectDescription[i].buffer(), 0, VK_WHOLE_SIZE);
        VkWriteDescriptorSet objectDescriptionWrite = vkinit::writeDescriptorSet(
            m_descriptorSetsTLAS[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, 1, &objectDescriptionDataDesriptor);

        std::vector<VkWriteDescriptorSet> writeSets = {descriptorWriteScene,
                                                       descriptorWriteModel,
                                                       descriptorWriteLightData,
                                                       descriptorWriteLightInstances,
                                                       objectDescriptionWrite};
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

    m_storageBufferObjectDescription.resize(nImages);
    for (int i = 0; i < nImages; i++) {
        VULKAN_CHECK_CRITICAL(createBuffer(m_vkctx.physicalDevice(),
                                           m_vkctx.device(),
                                           VULKAN_LIMITS_MAX_OBJECTS * sizeof(ObjectDescriptionPT),
                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           m_storageBufferObjectDescription[i]));
    }

    m_modelDataUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);
    m_lightDataUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);
    m_lightInstancesUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    return VK_SUCCESS;
}

void VulkanScene::createTopLevelAccelerationStructure(VulkanCommandInfo vci, uint32_t imageIndex)
{
    /* Create a buffer to hold top level instances */
    std::vector<VkAccelerationStructureInstanceKHR> instances(m_blasInstances.size());
    for (size_t i = 0; i < m_blasInstances.size(); i++) {
        auto &t = m_blasInstances[i].modelMatrix;
        /* Set transform matrix per blas */
        VkTransformMatrixKHR transformMatrix = {
            t[0][0], t[1][0], t[2][0], t[3][0], t[0][1], t[1][1], t[2][1], t[3][1], t[0][2], t[1][2], t[2][2], t[3][2]};

        instances[i].transform = transformMatrix;
        instances[i].instanceCustomIndex = static_cast<uint32_t>(i); /* The custom index specifies where the reference of this
        object
                                                                     is inside the array of references that is passed in the shader.
                                                                     More specifically, this reference will tell the shader where
                                                                     the geometry buffers for this object are, the mesh's position
                                                                     inside the ObjectDescription buffer */
        instances[i].mask = 0xFF;
        instances[i].instanceShaderBindingTableRecordOffset = m_blasInstances[i].instanceOffset;
        instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instances[i].accelerationStructureReference = m_blasInstances[i].accelerationStructure.buffer().address().deviceAddress;
    }

    m_tlas[imageIndex].initializeTopLevelAcceslerationStructure({vci.physicalDevice, vci.device, vci.commandPool, vci.queue},
                                                                instances);
}

}  // namespace vengine