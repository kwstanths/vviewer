#include "Materials.hpp"

void MaterialPBRStandard::setAlbedoTexture(std::shared_ptr<Texture> texture)
{
    m_albedoTexture = texture;
}

void MaterialPBRStandard::setMetallicTexture(std::shared_ptr<Texture> texture)
{
    m_metallicTexture = texture;
}

void MaterialPBRStandard::setRoughnessTexture(std::shared_ptr<Texture> texture)
{
    m_roughnessTexture = texture;
}

void MaterialPBRStandard::setAOTexture(std::shared_ptr<Texture> texture)
{
    m_aoTexture = texture;
}

void MaterialPBRStandard::setEmissiveTexture(std::shared_ptr<Texture> texture)
{
    m_emissiveTexture = texture;
}

void MaterialPBRStandard::setNormalTexture(std::shared_ptr<Texture> texture)
{
    m_normalTexture = texture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getAlbedoTexture() const
{
    return m_albedoTexture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getMetallicTexture() const
{
    return m_metallicTexture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getRoughnessTexture() const
{
    return m_roughnessTexture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getAOTexture() const
{
    return m_aoTexture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getEmissiveTexture() const
{
    return m_emissiveTexture;
}

std::shared_ptr<Texture> MaterialPBRStandard::getNormalTexture() const
{
    return m_normalTexture;
}

void MaterialLambert::setAlbedoTexture(std::shared_ptr<Texture> texture)
{
    m_albedoTexture = texture;
}

void MaterialLambert::setAOTexture(std::shared_ptr<Texture> texture)
{
    m_aoTexture = texture;
}

void MaterialLambert::setEmissiveTexture(std::shared_ptr<Texture> texture)
{
    m_emissiveTexture = texture;
}

void MaterialLambert::setNormalTexture(std::shared_ptr<Texture> texture)
{
    m_normalTexture = texture;
}

std::shared_ptr<Texture> MaterialLambert::getAlbedoTexture() const
{
    return m_albedoTexture;
}

std::shared_ptr<Texture> MaterialLambert::getAOTexture() const
{
    return m_aoTexture;
}

std::shared_ptr<Texture> MaterialLambert::getEmissiveTexture() const
{
    return m_emissiveTexture;
}

std::shared_ptr<Texture> MaterialLambert::getNormalTexture() const
{
    return m_normalTexture;
}

void MaterialSkybox::setMap(std::shared_ptr<EnvironmentMap> envMap)
{
    m_envMap = envMap;
}

std::shared_ptr<EnvironmentMap> MaterialSkybox::getMap() const
{
    return m_envMap;
}
