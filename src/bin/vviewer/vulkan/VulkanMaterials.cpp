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
    VulkanDynamicUBO<MaterialPBRData>& materialsDynamicUBO, int materialsUBOBlock)
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

    createSampler(device);
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
    allocInfo.descriptorSetCount = images;
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

    VkDescriptorImageInfo  texInfo[6];
    for (size_t t = 0; t < 6; t++) {
        texInfo[t] = VkDescriptorImageInfo();
        texInfo[t].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        texInfo[t].sampler = m_sampler;
    }
    texInfo[0].imageView = static_cast<VulkanTexture *>(m_albedoTexture)->getImageView();
    texInfo[1].imageView = static_cast<VulkanTexture *>(m_metallicTexture)->getImageView();
    texInfo[2].imageView = static_cast<VulkanTexture *>(m_roughnessTexture)->getImageView();
    texInfo[3].imageView = static_cast<VulkanTexture *>(m_aoTexture)->getImageView();
    texInfo[4].imageView = static_cast<VulkanTexture *>(m_emissiveTexture)->getImageView();
    texInfo[5].imageView = static_cast<VulkanTexture *>(m_normalTexture)->getImageView();

    VkWriteDescriptorSet descriptorWriteTextures{};
    descriptorWriteTextures.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteTextures.dstSet = m_descriptorSets[index];
    descriptorWriteTextures.dstBinding = 1;
    descriptorWriteTextures.dstArrayElement = 0;
    descriptorWriteTextures.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteTextures.descriptorCount = 6;
    descriptorWriteTextures.pImageInfo = texInfo;

    std::array<VkWriteDescriptorSet, 2> writeSets = { descriptorWriteMaterial, descriptorWriteTextures };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    m_descirptorsNeedUpdate[index] = false;

    return true;
}

bool VulkanMaterialPBR::createSampler(VkDevice device)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
        utils::ConsoleCritical("Failed to create a texture sampler");
        return false;
    }
    
    return true;
}

VulkanMaterialSkybox::VulkanMaterialSkybox(std::string name, Cubemap * cubemap) : MaterialSkybox(name)
{
    m_cubemap = cubemap;
}

void VulkanMaterialSkybox::setCubemap(Cubemap * cubemap)
{
    m_cubemap = cubemap;
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
    allocInfo.descriptorSetCount = images;
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
    VkDescriptorImageInfo cubemapInfo;
    cubemapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    cubemapInfo.sampler = static_cast<VulkanCubemap *>(m_cubemap)->getSampler();
    cubemapInfo.imageView = static_cast<VulkanCubemap *>(m_cubemap)->getImageView();
    VkWriteDescriptorSet descriptorWriteTextures{};
    descriptorWriteTextures.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteTextures.dstSet = m_descriptorSets[index];
    descriptorWriteTextures.dstBinding = 0;
    descriptorWriteTextures.dstArrayElement = 0;
    descriptorWriteTextures.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteTextures.descriptorCount = 1;
    descriptorWriteTextures.pImageInfo = &cubemapInfo;
    
    std::array<VkWriteDescriptorSet, 1> writeSets = { descriptorWriteTextures };
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);

    return true;
}
