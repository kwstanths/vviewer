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
                                                     VulkanUBO<MaterialData> &materialsUBO)
    : VulkanMaterialStorage<MaterialData>(materialsUBO)
    , MaterialPBRStandard(name)
{
    albedo() = a;
    metallic() = m;
    roughness() = r;
    ao() = ambient;
    emissive() = e;
    uTiling() = 1.F;
    vTiling() = 1.F;

    AssetManager<std::string, Texture> &instance = AssetManager<std::string, Texture>::getInstance();
    if (!instance.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!instance.isPresent("whiteColor")) {
        throw std::runtime_error("White color texture not present");
    }
    if (!instance.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    auto white = instance.get("white");
    auto whiteColor = instance.get("whiteColor");
    auto normalmap = instance.get("normalmapdefault");

    setAlbedoTexture(whiteColor);
    setMetallicTexture(white);
    setRoughnessTexture(white);
    setAOTexture(white);
    setEmissiveTexture(whiteColor);
    setNormalTexture(normalmap);

    if (!instance.isPresent("PBR_BRDF_LUT")) {
        throw std::runtime_error("PBR_BRDF_LUT texture not present");
    }
    auto BRDFLUT = std::static_pointer_cast<VulkanTexture>(instance.get("PBR_BRDF_LUT"));
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

glm::vec4 VulkanMaterialPBRStandard::getAlbedo() const
{
    return m_data->albedo;
}

float &VulkanMaterialPBRStandard::metallic()
{
    return m_data->metallicRoughnessAOEmissive.r;
}

float VulkanMaterialPBRStandard::getMetallic() const
{
    return m_data->metallicRoughnessAOEmissive.r;
}

float &VulkanMaterialPBRStandard::roughness()
{
    return m_data->metallicRoughnessAOEmissive.g;
}

float VulkanMaterialPBRStandard::getRoughness() const
{
    return m_data->metallicRoughnessAOEmissive.g;
}

float &VulkanMaterialPBRStandard::ao()
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float VulkanMaterialPBRStandard::getAO() const
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float &VulkanMaterialPBRStandard::emissive()
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float VulkanMaterialPBRStandard::getEmissive() const
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float &VulkanMaterialPBRStandard::uTiling()
{
    return m_data->uvTiling.r;
}

float VulkanMaterialPBRStandard::getUTiling() const
{
    return m_data->uvTiling.r;
}

float &VulkanMaterialPBRStandard::vTiling()
{
    return m_data->uvTiling.g;
}

float VulkanMaterialPBRStandard::getVTiling() const
{
    return m_data->uvTiling.g;
}

void VulkanMaterialPBRStandard::setAlbedoTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setAlbedoTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setMetallicTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setMetallicTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.g = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setRoughnessTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setRoughnessTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.b = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setAOTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setAOTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setEmissiveTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setEmissiveTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setNormalTexture(std::shared_ptr<Texture> texture)
{
    MaterialPBRStandard::setNormalTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialLambert::VulkanMaterialLambert(std::string name,
                                             glm::vec4 a,
                                             float ambient,
                                             float e,
                                             VkDevice device,
                                             VkDescriptorSetLayout descriptorLayout,
                                             VulkanUBO<MaterialData> &materialsUBO)
    : VulkanMaterialStorage<MaterialData>(materialsUBO)
    , MaterialLambert(name)
{
    albedo() = a;
    ao() = ambient;
    emissive() = e;
    uTiling() = 1.F;
    vTiling() = 1.F;

    AssetManager<std::string, Texture> &instance = AssetManager<std::string, Texture>::getInstance();
    if (!instance.isPresent("white")) {
        throw std::runtime_error("White texture not present");
    }
    if (!instance.isPresent("whiteColor")) {
        throw std::runtime_error("White color texture not present");
    }
    if (!instance.isPresent("normalmapdefault")) {
        throw std::runtime_error("Normal default texture not present");
    }

    auto white = instance.get("white");
    auto whiteColor = instance.get("whiteColor");
    auto normalmap = instance.get("normalmapdefault");

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

glm::vec4 VulkanMaterialLambert::getAlbedo() const
{
    return m_data->albedo;
}

float &VulkanMaterialLambert::ao()
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float VulkanMaterialLambert::getAO() const
{
    return m_data->metallicRoughnessAOEmissive.b;
}

float &VulkanMaterialLambert::emissive()
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float VulkanMaterialLambert::getEmissive() const
{
    return m_data->metallicRoughnessAOEmissive.a;
}

float &VulkanMaterialLambert::uTiling()
{
    return m_data->uvTiling.r;
}

float VulkanMaterialLambert::getUTiling() const
{
    return m_data->uvTiling.r;
}

float &VulkanMaterialLambert::vTiling()
{
    return m_data->uvTiling.g;
}

float VulkanMaterialLambert::getVTiling() const
{
    return m_data->uvTiling.g;
}

void VulkanMaterialLambert::setAlbedoTexture(std::shared_ptr<Texture> texture)
{
    MaterialLambert::setAlbedoTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setAOTexture(std::shared_ptr<Texture> texture)
{
    MaterialLambert::setAOTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setEmissiveTexture(std::shared_ptr<Texture> texture)
{
    MaterialLambert::setEmissiveTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setNormalTexture(std::shared_ptr<Texture> texture)
{
    MaterialLambert::setNormalTexture(texture);

    int32_t textureIndex = std::static_pointer_cast<VulkanTexture>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_data->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialSkybox::VulkanMaterialSkybox(std::string name,
                                           std::shared_ptr<EnvironmentMap> envMap,
                                           VkDevice device,
                                           VkDescriptorSetLayout descriptorLayout)
    : VulkanMaterialDescriptor(descriptorLayout)
    , MaterialSkybox(name)
{
    m_envMap = envMap;
}

void VulkanMaterialSkybox::setMap(std::shared_ptr<EnvironmentMap> envMap)
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
        vkinit::descriptorImageInfo(std::static_pointer_cast<VulkanCubemap>(m_envMap->getSkyboxMap())->getSampler(),
                                    std::static_pointer_cast<VulkanCubemap>(m_envMap->getSkyboxMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteSkybox =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &skyboxInfo);

    VkDescriptorImageInfo irradianceInfo =
        vkinit::descriptorImageInfo(std::static_pointer_cast<VulkanCubemap>(m_envMap->getIrradianceMap())->getSampler(),
                                    std::static_pointer_cast<VulkanCubemap>(m_envMap->getIrradianceMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteIrradiance =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, &irradianceInfo);

    VkDescriptorImageInfo prefilteredMapInfo =
        vkinit::descriptorImageInfo(std::static_pointer_cast<VulkanCubemap>(m_envMap->getPrefilteredMap())->getSampler(),
                                    std::static_pointer_cast<VulkanCubemap>(m_envMap->getPrefilteredMap())->getImageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWritePrefiltered =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, &prefilteredMapInfo);

    std::array<VkWriteDescriptorSet, 3> writeSets = {descriptorWriteSkybox, descriptorWriteIrradiance, descriptorWritePrefiltered};
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

}  // namespace vengine