#include "VulkanMaterials.hpp"

#include <core/AssetManager.hpp>

#include "vulkan/common/VulkanInitializers.hpp"
#include "vulkan/resources/VulkanTexture.hpp"
#include "vulkan/resources/VulkanCubemap.hpp"

#include <debug_tools/Console.hpp>

namespace vengine
{

VulkanMaterialDescriptor::VulkanMaterialDescriptor(VkDescriptorSetLayout descriptorSetLayout)
    : m_descriptorSetLayout(descriptorSetLayout)
{
}

VkDescriptorSet VulkanMaterialDescriptor::getDescriptor(size_t index) const
{
    return m_descriptorSets[index];
}

bool VulkanMaterialDescriptor::needsUpdate(size_t index) const
{
    return m_descirptorsNeedUpdate[index];
}

VulkanMaterialPBRStandard::VulkanMaterialPBRStandard(std::string name,
                                                     std::string filepath,
                                                     VkDevice device,
                                                     VkDescriptorSetLayout descriptorLayout,
                                                     VulkanUBO<MaterialData> &materialsUBO)
    : VulkanMaterialStorage<MaterialData>(materialsUBO)
    , MaterialPBRStandard(name, filepath)
{
    albedo() = glm::vec4(1, 1, 1, 1);
    metallic() = 0;
    roughness() = 1;
    ao() = 1;
    emissive() = glm::vec4(0, 0, 0, 1);
    uTiling() = 1;
    vTiling() = 1;

    auto &textures = AssetManager::getInstance().texturesMap();
    if (!textures.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!textures.isPresent("whiteColor")) {
        throw std::runtime_error("White color texture not present");
    }
    if (!textures.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    auto white = textures.get("white");
    auto whiteColor = textures.get("whiteColor");
    auto normalmap = textures.get("normalmapdefault");

    setAlbedoTexture(whiteColor);
    setMetallicTexture(white);
    setRoughnessTexture(white);
    setAOTexture(white);
    setEmissiveTexture(whiteColor);
    setNormalTexture(normalmap);

    if (!textures.isPresent("PBR_BRDF_LUT")) {
        throw std::runtime_error("PBR_BRDF_LUT texture not present");
    }
    auto BRDFLUT = static_cast<VulkanTexture *>(textures.get("PBR_BRDF_LUT"));
    m_data->gTexturesIndices2.b = static_cast<uint32_t>(BRDFLUT->getBindlessResourceIndex());
}

MaterialIndex VulkanMaterialPBRStandard::getMaterialIndex() const
{
    return getUBOBlockIndex();
}

glm::vec4 &VulkanMaterialPBRStandard::albedo()
{
    return m_data->albedo;
}

const glm::vec4 &VulkanMaterialPBRStandard::albedo() const
{
    return m_data->albedo;
}

float &VulkanMaterialPBRStandard::metallic()
{
    return m_data->metallicRoughnessAO.r;
}

const float &VulkanMaterialPBRStandard::metallic() const
{
    return m_data->metallicRoughnessAO.r;
}

float &VulkanMaterialPBRStandard::roughness()
{
    return m_data->metallicRoughnessAO.g;
}

const float &VulkanMaterialPBRStandard::roughness() const
{
    return m_data->metallicRoughnessAO.g;
}

float &VulkanMaterialPBRStandard::ao()
{
    return m_data->metallicRoughnessAO.b;
}

const float &VulkanMaterialPBRStandard::ao() const
{
    return m_data->metallicRoughnessAO.b;
}

glm::vec4 &VulkanMaterialPBRStandard::emissive()
{
    return m_data->emissive;
}

const glm::vec4 &VulkanMaterialPBRStandard::emissive() const
{
    return m_data->emissive;
}

float &VulkanMaterialPBRStandard::emissiveIntensity()
{
    return m_data->emissive.a;
}

const float &VulkanMaterialPBRStandard::emissiveIntensity() const
{
    return m_data->emissive.a;
}

glm::vec3 VulkanMaterialPBRStandard::emissiveColor() const
{
    return glm::vec3(m_data->emissive.r, m_data->emissive.g, m_data->emissive.b) * m_data->emissive.a;
}

float &VulkanMaterialPBRStandard::uTiling()
{
    return m_data->uvTiling.r;
}

const float &VulkanMaterialPBRStandard::uTiling() const
{
    return m_data->uvTiling.r;
}

float &VulkanMaterialPBRStandard::vTiling()
{
    return m_data->uvTiling.g;
}

const float &VulkanMaterialPBRStandard::vTiling() const
{
    return m_data->uvTiling.g;
}

void VulkanMaterialPBRStandard::setAlbedoTexture(Texture *texture)
{
    MaterialPBRStandard::setAlbedoTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setMetallicTexture(Texture *texture)
{
    MaterialPBRStandard::setMetallicTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.g = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setRoughnessTexture(Texture *texture)
{
    MaterialPBRStandard::setRoughnessTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.b = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setAOTexture(Texture *texture)
{
    MaterialPBRStandard::setAOTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setEmissiveTexture(Texture *texture)
{
    MaterialPBRStandard::setEmissiveTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setNormalTexture(Texture *texture)
{
    MaterialPBRStandard::setNormalTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialLambert::VulkanMaterialLambert(std::string name,
                                             std::string filepath,
                                             VkDevice device,
                                             VkDescriptorSetLayout descriptorLayout,
                                             VulkanUBO<MaterialData> &materialsUBO)
    : VulkanMaterialStorage<MaterialData>(materialsUBO)
    , MaterialLambert(name, filepath)
{
    albedo() = glm::vec4(1, 1, 1, 1);
    ao() = 1;
    emissive() = glm::vec4(0, 0, 0, 1);
    uTiling() = 1.F;
    vTiling() = 1.F;

    auto &textures = AssetManager::getInstance().texturesMap();
    if (!textures.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!textures.isPresent("whiteColor")) {
        throw std::runtime_error("White color texture not present");
    }
    if (!textures.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    auto white = textures.get("white");
    auto whiteColor = textures.get("whiteColor");
    auto normalmap = textures.get("normalmapdefault");

    setAlbedoTexture(whiteColor);
    setAOTexture(white);
    setEmissiveTexture(whiteColor);
    setNormalTexture(normalmap);
}

MaterialIndex VulkanMaterialLambert::getMaterialIndex() const
{
    return getUBOBlockIndex();
}

glm::vec4 &VulkanMaterialLambert::albedo()
{
    return m_data->albedo;
}

const glm::vec4 &VulkanMaterialLambert::albedo() const
{
    return m_data->albedo;
}

float &VulkanMaterialLambert::ao()
{
    return m_data->metallicRoughnessAO.b;
}

const float &VulkanMaterialLambert::ao() const
{
    return m_data->metallicRoughnessAO.b;
}

glm::vec4 &VulkanMaterialLambert::emissive()
{
    return m_data->emissive;
}

const glm::vec4 &VulkanMaterialLambert::emissive() const
{
    return m_data->emissive;
}

float &VulkanMaterialLambert::emissiveIntensity()
{
    return m_data->emissive.a;
}

const float &VulkanMaterialLambert::emissiveIntensity() const
{
    return m_data->emissive.a;
}

glm::vec3 VulkanMaterialLambert::emissiveColor() const
{
    return glm::vec3(m_data->emissive.r, m_data->emissive.g, m_data->emissive.b) * m_data->emissive.a;
}

float &VulkanMaterialLambert::uTiling()
{
    return m_data->uvTiling.r;
}

const float &VulkanMaterialLambert::uTiling() const
{
    return m_data->uvTiling.r;
}

float &VulkanMaterialLambert::vTiling()
{
    return m_data->uvTiling.g;
}

const float &VulkanMaterialLambert::vTiling() const
{
    return m_data->uvTiling.g;
}

void VulkanMaterialLambert::setAlbedoTexture(Texture *texture)
{
    MaterialLambert::setAlbedoTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setAOTexture(Texture *texture)
{
    MaterialLambert::setAOTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setEmissiveTexture(Texture *texture)
{
    MaterialLambert::setEmissiveTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setNormalTexture(Texture *texture)
{
    MaterialLambert::setNormalTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialSkybox::VulkanMaterialSkybox(std::string name,
                                           EnvironmentMap *envMap,
                                           VkDevice device,
                                           VkDescriptorSetLayout descriptorLayout)
    : VulkanMaterialDescriptor(descriptorLayout)
    , MaterialSkybox(name)
{
    m_envMap = envMap;
}

void VulkanMaterialSkybox::setMap(EnvironmentMap *envMap)
{
    m_envMap = envMap;
    std::fill(m_descirptorsNeedUpdate.begin(), m_descirptorsNeedUpdate.end(), true);
}

VkResult VulkanMaterialSkybox::createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images)
{
    /* Create descriptor sets */
    m_descriptorSets.resize(images);
    m_descirptorsNeedUpdate.resize(images, false);

    std::vector<VkDescriptorSetLayout> layouts(images, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocateInfo(pool, static_cast<uint32_t>(images), layouts.data());
    VULKAN_CHECK_CRITICAL(vkAllocateDescriptorSets(device, &allocInfo, m_descriptorSets.data()));

    return VK_SUCCESS;
}

void VulkanMaterialSkybox::updateDescriptorSets(VkDevice device, size_t images)
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
}

void VulkanMaterialSkybox::updateDescriptorSet(VkDevice device, size_t index)
{
    VkDescriptorImageInfo skyboxInfo =
        vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->getSkyboxMap())->getSampler(),
                                    static_cast<VulkanCubemap *>(m_envMap->getSkyboxMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteSkybox =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &skyboxInfo);

    VkDescriptorImageInfo irradianceInfo =
        vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->getIrradianceMap())->getSampler(),
                                    static_cast<VulkanCubemap *>(m_envMap->getIrradianceMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteIrradiance =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, &irradianceInfo);

    VkDescriptorImageInfo prefilteredMapInfo =
        vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->getPrefilteredMap())->getSampler(),
                                    static_cast<VulkanCubemap *>(m_envMap->getPrefilteredMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWritePrefiltered =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, &prefilteredMapInfo);

    std::array<VkWriteDescriptorSet, 3> writeSets = {descriptorWriteSkybox, descriptorWriteIrradiance, descriptorWritePrefiltered};
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

}  // namespace vengine