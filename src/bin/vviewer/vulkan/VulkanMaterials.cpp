#include "VulkanMaterials.hpp"

#include <core/AssetManager.hpp>

#include "VulkanTexture.hpp"
#include "VulkanCubemap.hpp"

#include <utils/Console.hpp>

VkDescriptorSet VulkanMaterialDescriptor::getDescriptor(size_t index)
{
    return m_descriptorSets[index];
}

bool VulkanMaterialDescriptor::needsUpdate(size_t index) const
{
    return m_descirptorsNeedUpdate[index];
}

VulkanMaterialPBR::VulkanMaterialPBR(std::string name,
    glm::vec4 albedo,
    float metallic, 
    float roughness, 
    float ao, 
    float emissive,
    VkDevice device,
    VulkanDynamicUBO<MaterialPBRData>& materialsDynamicUBO, 
    uint32_t materialsUBOBlock)
    : VulkanMaterialStorage<MaterialPBRData>(materialsDynamicUBO, materialsUBOBlock), MaterialPBR(name)
{
    getAlbedo() = albedo;
    getMetallic() = metallic;
    getRoughness() = roughness;
    getAO() = ao;
    getEmissive() = emissive;

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    if (!instance.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!instance.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    Texture * white = instance.Get("white");
    Texture * normalmap = instance.Get("normalmapdefault");

    setAlbedoTexture(white);
    setMetallicTexture(white);
    setRoughnessTexture(white);
    setAOTexture(white);
    setEmissiveTexture(white);
    setNormalTexture(normalmap);

    if (!instance.isPresent("PBR_BRDF_LUT")) {
        throw std::runtime_error("PBR_BRDF_LUT texture not present");
    }
    m_BRDFLUT = static_cast<VulkanTexture*>(instance.Get("PBR_BRDF_LUT"));
}

glm::vec4 & VulkanMaterialPBR::getAlbedo()
{
    return m_data->albedo;
}

float & VulkanMaterialPBR::getMetallic()
{
    return m_data->metallicRoughnessAOEmissive.r;
}

float & VulkanMaterialPBR::getRoughness()
{
    return m_data->metallicRoughnessAOEmissive.g;
}

float & VulkanMaterialPBR::getAO()
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float & VulkanMaterialPBR::getEmissive()
{
    return m_data->metallicRoughnessAOEmissive.a;
}

void VulkanMaterialPBR::setAlbedoTexture(Texture * texture)
{
    MaterialPBR::setAlbedoTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBR::setMetallicTexture(Texture * texture)
{
    MaterialPBR::setMetallicTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBR::setRoughnessTexture(Texture * texture)
{
    MaterialPBR::setRoughnessTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBR::setAOTexture(Texture * texture)
{
    MaterialPBR::setAOTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBR::setEmissiveTexture(Texture * texture)
{
    MaterialPBR::setEmissiveTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBR::setNormalTexture(Texture * texture)
{
    MaterialPBR::setNormalTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

bool VulkanMaterialPBR::createDescriptors(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool, size_t images)
{
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(images);
    allocInfo.pSetLayouts = layouts.data();
    if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate PBR material descriptor sets");
        return false;
    }
    return true;
}

bool VulkanMaterialPBR::updateDescriptorSets(VkDevice device, size_t images)
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
    return true;
}

bool VulkanMaterialPBR::updateDescriptorSet(VkDevice device, size_t index)
{
    VkDescriptorBufferInfo bufferInfoMaterial{};
    bufferInfoMaterial.buffer = m_materialsDataStorage.getBuffer(index);
    bufferInfoMaterial.offset = 0;
    bufferInfoMaterial.range = m_materialsDataStorage.getBlockSizeAligned();
    VkWriteDescriptorSet descriptorWriteMaterial{};
    descriptorWriteMaterial.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteMaterial.dstSet = m_descriptorSets[index];
    descriptorWriteMaterial.dstBinding = 0;
    descriptorWriteMaterial.dstArrayElement = 0;
    descriptorWriteMaterial.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    descriptorWriteMaterial.descriptorCount = 1;
    descriptorWriteMaterial.pBufferInfo = &bufferInfoMaterial;
    descriptorWriteMaterial.pImageInfo = nullptr; // Optional
    descriptorWriteMaterial.pTexelBufferView = nullptr; // Optional

    VkDescriptorImageInfo  texInfo[7];
    for (size_t t = 0; t < 7; t++) {
        texInfo[t] = VkDescriptorImageInfo();
        texInfo[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    texInfo[0].imageView = static_cast<VulkanTexture *>(m_albedoTexture)->getImageView();
    texInfo[0].sampler = static_cast<VulkanTexture*>(m_albedoTexture)->getSampler();
    texInfo[1].imageView = static_cast<VulkanTexture *>(m_metallicTexture)->getImageView();
    texInfo[1].sampler = static_cast<VulkanTexture*>(m_metallicTexture)->getSampler();
    texInfo[2].imageView = static_cast<VulkanTexture *>(m_roughnessTexture)->getImageView();
    texInfo[2].sampler = static_cast<VulkanTexture*>(m_roughnessTexture)->getSampler();
    texInfo[3].imageView = static_cast<VulkanTexture *>(m_aoTexture)->getImageView();
    texInfo[3].sampler = static_cast<VulkanTexture*>(m_aoTexture)->getSampler();
    texInfo[4].imageView = static_cast<VulkanTexture *>(m_emissiveTexture)->getImageView();
    texInfo[4].sampler = static_cast<VulkanTexture*>(m_emissiveTexture)->getSampler();
    texInfo[5].imageView = static_cast<VulkanTexture*>(m_normalTexture)->getImageView();
    texInfo[5].sampler = static_cast<VulkanTexture*>(m_normalTexture)->getSampler();
    texInfo[6].imageView = m_BRDFLUT->getImageView();
    texInfo[6].sampler = m_BRDFLUT->getSampler();

    VkWriteDescriptorSet descriptorWriteTextures{};
    descriptorWriteTextures.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteTextures.dstSet = m_descriptorSets[index];
    descriptorWriteTextures.dstBinding = 1;
    descriptorWriteTextures.dstArrayElement = 0;
    descriptorWriteTextures.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteTextures.descriptorCount = 7;
    descriptorWriteTextures.pImageInfo = texInfo;

    std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteMaterial, descriptorWriteTextures };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_descirptorsNeedUpdate[index] = false;

    return true;
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialSkybox::VulkanMaterialSkybox(std::string name, EnvironmentMap * envMap, VkDevice device) : MaterialSkybox(name)
{
    m_envMap = envMap;
}

void VulkanMaterialSkybox::setMap(EnvironmentMap* envMap)
{
    m_envMap = envMap;
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

bool VulkanMaterialSkybox::createDescriptors(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool, size_t images)
{
    /* Create descriptor sets */
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, layout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(images);
    allocInfo.pSetLayouts = layouts.data();
    if (vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to allocate descriptor sets for cubemap data");
        return false;
    }

    return true;
}

bool VulkanMaterialSkybox::updateDescriptorSets(VkDevice device, size_t images)
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
    return true;
}

bool VulkanMaterialSkybox::updateDescriptorSet(VkDevice device, size_t index)
{
    VkDescriptorImageInfo skyboxInfo;
    skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    skyboxInfo.sampler = static_cast<VulkanCubemap*>(m_envMap->getSkyboxMap())->getSampler();
    skyboxInfo.imageView = static_cast<VulkanCubemap*>(m_envMap->getSkyboxMap())->getImageView();
    VkWriteDescriptorSet descriptorWriteSkybox{};
    descriptorWriteSkybox.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteSkybox.dstSet = m_descriptorSets[index];
    descriptorWriteSkybox.dstBinding = 0;
    descriptorWriteSkybox.dstArrayElement = 0;
    descriptorWriteSkybox.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteSkybox.descriptorCount = 1;
    descriptorWriteSkybox.pImageInfo = &skyboxInfo;

    VkDescriptorImageInfo irradianceInfo;
    irradianceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    irradianceInfo.sampler = static_cast<VulkanCubemap*>(m_envMap->getIrradianceMap())->getSampler();
    irradianceInfo.imageView = static_cast<VulkanCubemap*>(m_envMap->getIrradianceMap())->getImageView();
    VkWriteDescriptorSet descriptorWriteIrradiance{};
    descriptorWriteIrradiance.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteIrradiance.dstSet = m_descriptorSets[index];
    descriptorWriteIrradiance.dstBinding = 1;
    descriptorWriteIrradiance.dstArrayElement = 0;
    descriptorWriteIrradiance.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteIrradiance.descriptorCount = 1;
    descriptorWriteIrradiance.pImageInfo = &irradianceInfo;

    VkDescriptorImageInfo prefilteredMapInfo;
    prefilteredMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    prefilteredMapInfo.sampler = static_cast<VulkanCubemap*>(m_envMap->getPrefilteredMap())->getSampler();
    prefilteredMapInfo.imageView = static_cast<VulkanCubemap*>(m_envMap->getPrefilteredMap())->getImageView();
    VkWriteDescriptorSet descriptorWritePrefiltered{};
    descriptorWritePrefiltered.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWritePrefiltered.dstSet = m_descriptorSets[index];
    descriptorWritePrefiltered.dstBinding = 2;
    descriptorWritePrefiltered.dstArrayElement = 0;
    descriptorWritePrefiltered.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWritePrefiltered.descriptorCount = 1;
    descriptorWritePrefiltered.pImageInfo = &prefilteredMapInfo;
    
    std::array<VkWriteDescriptorSet, 3> writeSets = { descriptorWriteSkybox, descriptorWriteIrradiance, descriptorWritePrefiltered};
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    return true;
}

