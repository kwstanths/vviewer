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

VulkanMaterialPBRStandard::VulkanMaterialPBRStandard(std::string name,
    glm::vec4 a,
    float m, 
    float r, 
    float ambient, 
    float e,
    VkDevice device,
    VkDescriptorSetLayout descriptorLayout,
    VulkanDynamicUBO<MaterialData>& materialsDynamicUBO,
    uint32_t materialsUBOBlock)
    : VulkanMaterialStorage<MaterialData>(materialsDynamicUBO, materialsUBOBlock), MaterialPBRStandard(name)
{
    m_descriptorSetLayout = descriptorLayout;

    albedo() = a;
    metallic() = m;
    roughness() = r;
    ao() = ambient;
    emissive() = e;

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

glm::vec4 & VulkanMaterialPBRStandard::albedo()
{
    return m_data->albedo;
}

glm::vec4 VulkanMaterialPBRStandard::getAlbedo() const
{
    return m_data->albedo;
}

float & VulkanMaterialPBRStandard::metallic()
{
    return m_data->metallicRoughnessAOEmissive.r;
}

float VulkanMaterialPBRStandard::getMetallic() const
{
    return m_data->metallicRoughnessAOEmissive.r;
}

float & VulkanMaterialPBRStandard::roughness()
{
    return m_data->metallicRoughnessAOEmissive.g;
}

float VulkanMaterialPBRStandard::getRoughness() const
{
    return m_data->metallicRoughnessAOEmissive.g;
}

float & VulkanMaterialPBRStandard::ao()
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float VulkanMaterialPBRStandard::getAO() const
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float & VulkanMaterialPBRStandard::emissive()
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float VulkanMaterialPBRStandard::getEmissive() const
{
    return m_data->metallicRoughnessAOEmissive.a;
}

void VulkanMaterialPBRStandard::setAlbedoTexture(Texture * texture)
{
    MaterialPBRStandard::setAlbedoTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBRStandard::setMetallicTexture(Texture * texture)
{
    MaterialPBRStandard::setMetallicTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBRStandard::setRoughnessTexture(Texture * texture)
{
    MaterialPBRStandard::setRoughnessTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBRStandard::setAOTexture(Texture * texture)
{
    MaterialPBRStandard::setAOTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBRStandard::setEmissiveTexture(Texture * texture)
{
    MaterialPBRStandard::setEmissiveTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialPBRStandard::setNormalTexture(Texture * texture)
{
    MaterialPBRStandard::setNormalTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

bool VulkanMaterialPBRStandard::createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images)
{
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, m_descriptorSetLayout);
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

bool VulkanMaterialPBRStandard::updateDescriptorSets(VkDevice device, size_t images)
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
    return true;
}

bool VulkanMaterialPBRStandard::updateDescriptorSet(VkDevice device, size_t index)
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


VulkanMaterialLambert::VulkanMaterialLambert(std::string name, 
    glm::vec4 a, 
    float ambient, 
    float e, 
    VkDevice device, 
    VkDescriptorSetLayout descriptorLayout,
    VulkanDynamicUBO<MaterialData>& materialsDynamicUBO, 
    uint32_t materialsUBOBlock)
    :VulkanMaterialStorage<MaterialData>(materialsDynamicUBO, materialsUBOBlock), MaterialLambert(name)
{
    m_descriptorSetLayout = descriptorLayout;

    albedo() = a;
    ao() = ambient;
    emissive() = e;

    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    if (!instance.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!instance.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    Texture* white = instance.Get("white");
    Texture* normalmap = instance.Get("normalmapdefault");

    setAlbedoTexture(white);
    setAOTexture(white);
    setEmissiveTexture(white);
    setNormalTexture(normalmap);
}

glm::vec4& VulkanMaterialLambert::albedo()
{
    return m_data->albedo;
}

glm::vec4 VulkanMaterialLambert::getAlbedo() const
{
    return m_data->albedo;
}

float& VulkanMaterialLambert::ao()
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float VulkanMaterialLambert::getAO() const
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float& VulkanMaterialLambert::emissive()
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float VulkanMaterialLambert::getEmissive() const
{
    return m_data->metallicRoughnessAOEmissive.a;
}

void VulkanMaterialLambert::setAlbedoTexture(Texture* texture)
{
    MaterialLambert::setAlbedoTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialLambert::setAOTexture(Texture* texture)
{
    MaterialLambert::setAOTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialLambert::setEmissiveTexture(Texture* texture)
{
    MaterialLambert::setEmissiveTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

void VulkanMaterialLambert::setNormalTexture(Texture* texture)
{
    MaterialLambert::setNormalTexture(texture);
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

bool VulkanMaterialLambert::createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images)
{
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, m_descriptorSetLayout);
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

bool VulkanMaterialLambert::updateDescriptorSets(VkDevice device, size_t images)
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
    return true;
}

bool VulkanMaterialLambert::updateDescriptorSet(VkDevice device, size_t index)
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

    VkDescriptorImageInfo  texInfo[4];
    for (size_t t = 0; t < 4; t++) {
        texInfo[t] = VkDescriptorImageInfo();
        texInfo[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    texInfo[0].imageView = static_cast<VulkanTexture*>(m_albedoTexture)->getImageView();
    texInfo[0].sampler = static_cast<VulkanTexture*>(m_albedoTexture)->getSampler();
    texInfo[1].imageView = static_cast<VulkanTexture*>(m_aoTexture)->getImageView();
    texInfo[1].sampler = static_cast<VulkanTexture*>(m_aoTexture)->getSampler();
    texInfo[2].imageView = static_cast<VulkanTexture*>(m_emissiveTexture)->getImageView();
    texInfo[2].sampler = static_cast<VulkanTexture*>(m_emissiveTexture)->getSampler();
    texInfo[3].imageView = static_cast<VulkanTexture*>(m_normalTexture)->getImageView();
    texInfo[3].sampler = static_cast<VulkanTexture*>(m_normalTexture)->getSampler();

    VkWriteDescriptorSet descriptorWriteTextures{};
    descriptorWriteTextures.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteTextures.dstSet = m_descriptorSets[index];
    descriptorWriteTextures.dstBinding = 1;
    descriptorWriteTextures.dstArrayElement = 0;
    descriptorWriteTextures.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteTextures.descriptorCount = 4;
    descriptorWriteTextures.pImageInfo = texInfo;

    std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteMaterial, descriptorWriteTextures };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_descirptorsNeedUpdate[index] = false;

    return true;
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialSkybox::VulkanMaterialSkybox(std::string name, EnvironmentMap* envMap, VkDevice device, VkDescriptorSetLayout descriptorLayout) 
    : MaterialSkybox(name)
{
    m_descriptorSetLayout = descriptorLayout;

    m_envMap = envMap;
}

void VulkanMaterialSkybox::setMap(EnvironmentMap* envMap)
{
    m_envMap = envMap;
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

bool VulkanMaterialSkybox::createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images)
{
    /* Create descriptor sets */
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, m_descriptorSetLayout);
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

    std::array<VkWriteDescriptorSet, 3> writeSets = { descriptorWriteSkybox, descriptorWriteIrradiance, descriptorWritePrefiltered };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    return true;
}