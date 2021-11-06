#include "VulkanMaterials.hpp"

#include <core/AssetManager.hpp>

VulkanMaterialPBR::VulkanMaterialPBR(std::string name,
    glm::vec4 albedo,
    float metallic, 
    float roughness, 
    float ao, 
    float emissive,
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

    Texture * white = instance.Get("white");

    setAlbedoTexture(white);
    setMetallicTexture(white);
    setRoughnessTexture(white);
    setAOTexture(white);
    setEmissiveTexture(white);
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
}

void VulkanMaterialPBR::setMetallicTexture(Texture * texture)
{
    MaterialPBR::setMetallicTexture(texture);
}

void VulkanMaterialPBR::setRoughnessTexture(Texture * texture)
{
    MaterialPBR::setRoughnessTexture(texture);
}

void VulkanMaterialPBR::setAOTexture(Texture * texture)
{
    MaterialPBR::setAOTexture(texture);
}

void VulkanMaterialPBR::setEmissiveTexture(Texture * texture)
{
    MaterialPBR::setEmissiveTexture(texture);
}
