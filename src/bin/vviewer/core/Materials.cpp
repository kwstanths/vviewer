#include "Materials.hpp"

void MaterialPBR::setAlbedoTexture(Texture * texture)
{
    m_albedoTexture = texture;
}

void MaterialPBR::setMetallicTexture(Texture * texture)
{
    m_metallicTexture = texture;
}

void MaterialPBR::setRoughnessTexture(Texture * texture)
{
    m_roughnessTexture = texture;
}

void MaterialPBR::setAOTexture(Texture * texture)
{
    m_aoTexture = texture;
}

void MaterialPBR::setEmissiveTexture(Texture * texture)
{
    m_emissiveTexture = texture;
}

Texture * MaterialPBR::getAlbedoTexture() const
{
    return m_albedoTexture;
}

Texture * MaterialPBR::getMetallicTexture() const
{
    return m_metallicTexture;
}

Texture * MaterialPBR::getRoughnessTexture() const
{
    return m_roughnessTexture;
}

Texture * MaterialPBR::getAOTexture() const
{
    return m_aoTexture;
}

Texture * MaterialPBR::getEmissiveTexture() const
{
    return m_emissiveTexture;
}
