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

VulkanMaterialPBRStandard::VulkanMaterialPBRStandard(const AssetInfo &info, VulkanUBODefault<MaterialData> &materialsUBO)
    : VulkanUBODefault<MaterialData>::Block(materialsUBO)
    , MaterialPBRStandard(info)
{
    albedo() = glm::vec4(1, 1, 1, 1);
    metallic() = 0;
    roughness() = 1;
    ao() = 1;
    emissive() = glm::vec4(0, 0, 0, 1);
    uTiling() = 1;
    vTiling() = 1;

    auto &textures = AssetManager::getInstance().texturesMap();
    auto white = textures.get("white");
    auto whiteColor = textures.get("whiteColor");
    auto normalmap = textures.get("normalmapdefault");

    setAlbedoTexture(whiteColor);
    setMetallicTexture(white);
    setRoughnessTexture(white);
    setAOTexture(white);
    setEmissiveTexture(whiteColor);
    setNormalTexture(normalmap);
    setAlphaTexture(white);

    auto BRDFLUT = static_cast<VulkanTexture *>(textures.get("PBR_BRDF_LUT"));
    m_block->gTexturesIndices2.b = static_cast<uint32_t>(BRDFLUT->getBindlessResourceIndex());
}

MaterialIndex VulkanMaterialPBRStandard::materialIndex() const
{
    return UBOBlockIndex();
}

bool VulkanMaterialPBRStandard::isTransparent() const
{
    return m_block->metallicRoughnessAO.a > 0;
}

void VulkanMaterialPBRStandard::setTransparent(bool transparent)
{
    m_block->metallicRoughnessAO.a = static_cast<bool>(transparent);
}

glm::vec4 &VulkanMaterialPBRStandard::albedo()
{
    return m_block->albedo;
}

const glm::vec4 &VulkanMaterialPBRStandard::albedo() const
{
    return m_block->albedo;
}

float &VulkanMaterialPBRStandard::metallic()
{
    return m_block->metallicRoughnessAO.r;
}

const float &VulkanMaterialPBRStandard::metallic() const
{
    return m_block->metallicRoughnessAO.r;
}

float &VulkanMaterialPBRStandard::roughness()
{
    return m_block->metallicRoughnessAO.g;
}

const float &VulkanMaterialPBRStandard::roughness() const
{
    return m_block->metallicRoughnessAO.g;
}

float &VulkanMaterialPBRStandard::ao()
{
    return m_block->metallicRoughnessAO.b;
}

const float &VulkanMaterialPBRStandard::ao() const
{
    return m_block->metallicRoughnessAO.b;
}

glm::vec4 &VulkanMaterialPBRStandard::emissive()
{
    return m_block->emissive;
}

const glm::vec4 &VulkanMaterialPBRStandard::emissive() const
{
    return m_block->emissive;
}

float &VulkanMaterialPBRStandard::emissiveIntensity()
{
    return m_block->emissive.a;
}

const float &VulkanMaterialPBRStandard::emissiveIntensity() const
{
    return m_block->emissive.a;
}

glm::vec3 VulkanMaterialPBRStandard::emissiveColor() const
{
    return glm::vec3(m_block->emissive.r, m_block->emissive.g, m_block->emissive.b) * m_block->emissive.a;
}

float &VulkanMaterialPBRStandard::uTiling()
{
    return m_block->uvTiling.r;
}

const float &VulkanMaterialPBRStandard::uTiling() const
{
    return m_block->uvTiling.r;
}

float &VulkanMaterialPBRStandard::vTiling()
{
    return m_block->uvTiling.g;
}

const float &VulkanMaterialPBRStandard::vTiling() const
{
    return m_block->uvTiling.g;
}

void VulkanMaterialPBRStandard::setAlbedoTexture(Texture *texture)
{
    MaterialPBRStandard::setAlbedoTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setMetallicTexture(Texture *texture)
{
    MaterialPBRStandard::setMetallicTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.g = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setRoughnessTexture(Texture *texture)
{
    MaterialPBRStandard::setRoughnessTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.b = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setAOTexture(Texture *texture)
{
    MaterialPBRStandard::setAOTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setEmissiveTexture(Texture *texture)
{
    MaterialPBRStandard::setEmissiveTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setNormalTexture(Texture *texture)
{
    MaterialPBRStandard::setNormalTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialPBRStandard::setAlphaTexture(Texture *texture)
{
    MaterialPBRStandard::setAlphaTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.a = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialLambert::VulkanMaterialLambert(const AssetInfo &info, VulkanUBODefault<MaterialData> &materialsUBO)
    : VulkanUBODefault<MaterialData>::Block(materialsUBO)
    , MaterialLambert(info)
{
    albedo() = glm::vec4(1, 1, 1, 1);
    ao() = 1;
    emissive() = glm::vec4(0, 0, 0, 1);
    uTiling() = 1.F;
    vTiling() = 1.F;

    auto &textures = AssetManager::getInstance().texturesMap();

    auto white = textures.get("white");
    auto whiteColor = textures.get("whiteColor");
    auto normalmap = textures.get("normalmapdefault");

    setAlbedoTexture(whiteColor);
    setAOTexture(white);
    setEmissiveTexture(whiteColor);
    setNormalTexture(normalmap);
    setAlphaTexture(white);
}

MaterialIndex VulkanMaterialLambert::materialIndex() const
{
    return UBOBlockIndex();
}

bool VulkanMaterialLambert::isTransparent() const
{
    return m_block->metallicRoughnessAO.a > 0;
}

void VulkanMaterialLambert::setTransparent(bool transparent)
{
    m_block->metallicRoughnessAO.a = static_cast<bool>(transparent);
}

glm::vec4 &VulkanMaterialLambert::albedo()
{
    return m_block->albedo;
}

const glm::vec4 &VulkanMaterialLambert::albedo() const
{
    return m_block->albedo;
}

float &VulkanMaterialLambert::ao()
{
    return m_block->metallicRoughnessAO.b;
}

const float &VulkanMaterialLambert::ao() const
{
    return m_block->metallicRoughnessAO.b;
}

glm::vec4 &VulkanMaterialLambert::emissive()
{
    return m_block->emissive;
}

const glm::vec4 &VulkanMaterialLambert::emissive() const
{
    return m_block->emissive;
}

float &VulkanMaterialLambert::emissiveIntensity()
{
    return m_block->emissive.a;
}

const float &VulkanMaterialLambert::emissiveIntensity() const
{
    return m_block->emissive.a;
}

glm::vec3 VulkanMaterialLambert::emissiveColor() const
{
    return glm::vec3(m_block->emissive.r, m_block->emissive.g, m_block->emissive.b) * m_block->emissive.a;
}

float &VulkanMaterialLambert::uTiling()
{
    return m_block->uvTiling.r;
}

const float &VulkanMaterialLambert::uTiling() const
{
    return m_block->uvTiling.r;
}

float &VulkanMaterialLambert::vTiling()
{
    return m_block->uvTiling.g;
}

const float &VulkanMaterialLambert::vTiling() const
{
    return m_block->uvTiling.g;
}

void VulkanMaterialLambert::setAlbedoTexture(Texture *texture)
{
    MaterialLambert::setAlbedoTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setAOTexture(Texture *texture)
{
    MaterialLambert::setAOTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices1.a = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setEmissiveTexture(Texture *texture)
{
    MaterialLambert::setEmissiveTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.r = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setNormalTexture(Texture *texture)
{
    MaterialLambert::setNormalTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.g = static_cast<uint32_t>(textureIndex);
}

void VulkanMaterialLambert::setAlphaTexture(Texture *texture)
{
    MaterialLambert::setAlphaTexture(texture);

    int32_t textureIndex = static_cast<VulkanTexture *>(texture)->getBindlessResourceIndex();
    assert(textureIndex >= 0);
    m_block->gTexturesIndices2.a = static_cast<uint32_t>(textureIndex);
}

/* ------------------------------------------------------------------------------------------------------------------- */

VulkanMaterialSkybox::VulkanMaterialSkybox(const AssetInfo &info, EnvironmentMap *envMap, VkDescriptorSetLayout descriptorLayout)
    : VulkanMaterialDescriptor(descriptorLayout)
    , MaterialSkybox(info)
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

void VulkanMaterialSkybox::updateDescriptorSets(VkDevice device, size_t images) const
{
    /* Write descriptor sets */
    for (size_t i = 0; i < images; i++) {
        updateDescriptorSet(device, i);
    }
}

void VulkanMaterialSkybox::updateDescriptorSet(VkDevice device, size_t index) const
{
    VkDescriptorImageInfo skyboxInfo = vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->skyboxMap())->sampler(),
                                                                   static_cast<VulkanCubemap *>(m_envMap->skyboxMap())->imageView(),
                                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteSkybox =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 1, &skyboxInfo);

    VkDescriptorImageInfo irradianceInfo =
        vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->irradianceMap())->sampler(),
                                    static_cast<VulkanCubemap *>(m_envMap->irradianceMap())->imageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWriteIrradiance =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, &irradianceInfo);

    VkDescriptorImageInfo prefilteredMapInfo =
        vkinit::descriptorImageInfo(static_cast<VulkanCubemap *>(m_envMap->prefilteredMap())->sampler(),
                                    static_cast<VulkanCubemap *>(m_envMap->prefilteredMap())->imageView(),
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    VkWriteDescriptorSet descriptorWritePrefiltered =
        vkinit::writeDescriptorSet(m_descriptorSets[index], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, 1, &prefilteredMapInfo);

    std::array<VkWriteDescriptorSet, 3> writeSets = {descriptorWriteSkybox, descriptorWriteIrradiance, descriptorWritePrefiltered};
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

VulkanMaterialVolume::VulkanMaterialVolume(const AssetInfo &info, VulkanUBODefault<MaterialData> &materialsUBO)
    : MaterialVolume(info)
    , VulkanUBODefault<MaterialData>::Block(materialsUBO)
{
    sigmaS() = glm::vec4(0.2);
    sigmaA() = glm::vec4(0.01);
    g() = 0.0F;
};

MaterialIndex VulkanMaterialVolume::materialIndex() const
{
    return UBOBlockIndex();
}

glm::vec4 &VulkanMaterialVolume::sigmaA()
{
    return m_block->albedo;
}

const glm::vec4 &VulkanMaterialVolume::sigmaA() const
{
    return m_block->albedo;
}

glm::vec4 &VulkanMaterialVolume::sigmaS()
{
    return m_block->metallicRoughnessAO;
}

const glm::vec4 &VulkanMaterialVolume::sigmaS() const
{
    return m_block->metallicRoughnessAO;
}

float &VulkanMaterialVolume::g()
{
    return m_block->emissive.r;
}

float &VulkanMaterialVolume::g() const
{
    return m_block->emissive.r;
}

}  // namespace vengine