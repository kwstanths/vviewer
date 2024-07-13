#include "VulkanInstances.hpp"

#include "vulkan/common/VulkanLimits.hpp"
#include "vulkan/resources/VulkanMesh.hpp"

namespace vengine
{

VulkanInstancesManager::VulkanInstancesManager(VulkanContext &vkctx)
    : m_vkctx(vkctx)
    , m_instancesSSBO(VULKAN_LIMITS_MAX_OBJECTS)
    , m_lightInstancesUBO(VULKAN_LIMITS_MAX_LIGHT_INSTANCES)
    , InstancesManager()
{
}

VulkanInstancesManager::~VulkanInstancesManager()
{
}

VkResult VulkanInstancesManager::initResources()
{
    /* Initialize uniform buffers for light instances */
    m_lightInstancesUBO.init(m_vkctx.physicalDeviceProperties());

    VULKAN_CHECK_CRITICAL(m_instancesSSBO.init(m_vkctx.physicalDeviceProperties()));
    InstancesManager::initResources(m_instancesSSBO.block(0), m_instancesSSBO.nblocks());

    VULKAN_CHECK_CRITICAL(createDescriptorSetsLayouts());

    return VK_SUCCESS;
}

VkResult VulkanInstancesManager::initSwapchainResources(uint32_t nImages)
{
    m_lightInstancesUBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages);

    VULKAN_CHECK_CRITICAL(m_instancesSSBO.createBuffers(m_vkctx.physicalDevice(), m_vkctx.device(), nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorPool(nImages));
    VULKAN_CHECK_CRITICAL(createDescriptorSets(nImages));

    return VK_SUCCESS;
}

VkResult VulkanInstancesManager::releaseResources()
{
    m_lightInstancesUBO.destroyCPUMemory();
    m_instancesSSBO.destroyCPUMemory();

    vkDestroyDescriptorSetLayout(m_vkctx.device(), m_descriptorSetLayoutInstanceData, nullptr);

    return VK_SUCCESS;
}

VkResult VulkanInstancesManager::releaseSwapchainResources()
{
    m_lightInstancesUBO.destroyGPUBuffers(m_vkctx.device());
    m_instancesSSBO.destroyGPUBuffers(m_vkctx.device());

    vkDestroyDescriptorPool(m_vkctx.device(), m_descriptorPool, nullptr);

    return VK_SUCCESS;
}

void VulkanInstancesManager::build(SceneGraph &sceneGraph)
{
    InstancesManager::build(sceneGraph);

    /* Update light instances */
    for (uint32_t l = 0; l < m_lights.size(); l++) {
        assert(l < m_lightInstancesUBO.nblocks());

        SceneObject *so = m_lights[l];
        Light *light = so->get<ComponentLight>().light;
        assert(light != nullptr);

        LightInstance *lightInstanceBlock = m_lightInstancesUBO.block(l);
        lightInstanceBlock->info.r = light->lightIndex();
        lightInstanceBlock->info.g = getInstanceDataIndex(so); /* This is not used */
        lightInstanceBlock->info.a = static_cast<int32_t>(light->type());

        if (light->type() == LightType::POINT_LIGHT) {
            lightInstanceBlock->position = glm::vec4(so->worldPosition(), LightType::POINT_LIGHT);
        } else if (light->type() == LightType::DIRECTIONAL_LIGHT) {
            lightInstanceBlock->position = glm::vec4(so->modelMatrix() * glm::vec4(Transform::WORLD_Z, 0));
        }
        lightInstanceBlock->position.a = static_cast<float>(so->get<ComponentLight>().castShadows);
    }
    for (uint32_t l = 0; l < m_meshLights.size(); l++) {
        uint32_t nl = m_lights.size() + l;
        assert(nl < m_lightInstancesUBO.nblocks());

        SceneObject *so = m_meshLights[l];
        Material *material = so->get<ComponentMaterial>().material;
        assert(material != nullptr);

        LightInstance *lightInstanceBlock = m_lightInstancesUBO.block(nl);
        lightInstanceBlock->info.g = getInstanceDataIndex(so); /* This is not used */
        lightInstanceBlock->info.a = static_cast<int32_t>(LightType::MESH_LIGHT);

        const auto &m = so->modelMatrix();
        lightInstanceBlock->position = glm::vec4(m[0][0], m[1][0], m[2][0], m[3][0]);
        lightInstanceBlock->position1 = glm::vec4(m[0][1], m[1][1], m[2][1], m[3][1]);
        lightInstanceBlock->position2 = glm::vec4(m[0][2], m[1][2], m[2][2], m[3][2]);
    }
}

void VulkanInstancesManager::updateBuffers(uint32_t imageIndex)
{
    m_lightInstancesUBO.updateBuffer(m_vkctx.device(), imageIndex);
    m_instancesSSBO.updateBuffer(m_vkctx.device(), imageIndex);
}

void VulkanInstancesManager::setInstanceData(InstanceData *instanceData, SceneObject *so)
{
    InstancesManager::setInstanceData(instanceData, so);

    if (so->has<ComponentMesh>()) {
        auto mesh = static_cast<VulkanMesh *>(so->get<ComponentMesh>().mesh);
        assert(mesh->blas().initialized());

        instanceData->vertexAddress = mesh->vertexBuffer().address().deviceAddress;
        instanceData->indexAddress = mesh->indexBuffer().address().deviceAddress;
        instanceData->numTriangles = mesh->nTriangles();
    }
}

VkResult VulkanInstancesManager::createDescriptorSetsLayouts()
{
    /* Binding 0, instance data buffer */
    VkDescriptorSetLayoutBinding instanceDataBufferBinding =
        vkinit::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT |
                                               VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
                                           0,
                                           1);

    std::vector<VkDescriptorSetLayoutBinding> bindings({instanceDataBufferBinding});
    VkDescriptorSetLayoutCreateInfo descriptorSetlayoutInfo =
        vkinit::descriptorSetLayoutCreateInfo(static_cast<uint32_t>(bindings.size()), bindings.data());
    VULKAN_CHECK_CRITICAL(
        vkCreateDescriptorSetLayout(m_vkctx.device(), &descriptorSetlayoutInfo, nullptr, &m_descriptorSetLayoutInstanceData));

    return VK_SUCCESS;
}

VkResult VulkanInstancesManager::createDescriptorPool(uint32_t nImages)
{
    VkDescriptorPoolSize instanceDataPoolSize = vkinit::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nImages);
    std::vector<VkDescriptorPoolSize> poolSizes = {instanceDataPoolSize};

    VkDescriptorPoolCreateInfo poolInfo =
        vkinit::descriptorPoolCreateInfo(static_cast<uint32_t>(poolSizes.size()),
                                         poolSizes.data(),
                                         static_cast<uint32_t>(2 * nImages + VULKAN_LIMITS_MAX_MATERIALS * nImages));
    VULKAN_CHECK_CRITICAL(vkCreateDescriptorPool(m_vkctx.device(), &poolInfo, nullptr, &m_descriptorPool));

    return VK_SUCCESS;
}

VkResult VulkanInstancesManager::createDescriptorSets(uint32_t nImages)
{
    /* Create descriptor sets */
    {
        m_descriptorSetsInstanceData.resize(nImages);

        std::vector<VkDescriptorSetLayout> layouts(nImages, m_descriptorSetLayoutInstanceData);
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(m_descriptorPool, nImages, layouts.data());

        VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(m_vkctx.device(), &allocInfo, m_descriptorSetsInstanceData.data()));
    }

    /* Write descriptor sets */
    for (size_t i = 0; i < nImages; i++) {
        /* InstanceData */
        auto bufferInfoInstanceData = vkinit::descriptorBufferInfo(m_instancesSSBO.buffer(i), 0, VK_WHOLE_SIZE);
        auto descriptorWriteInstanceData = vkinit::writeDescriptorSet(
            m_descriptorSetsInstanceData[i], VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 1, &bufferInfoInstanceData);

        std::vector<VkWriteDescriptorSet> writeSets = {descriptorWriteInstanceData};
        vkUpdateDescriptorSets(m_vkctx.device(), static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
    }

    return VK_SUCCESS;
}

}  // namespace vengine