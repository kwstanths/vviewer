#include "Material.hpp"

#include "vengine/math/MathUtils.hpp"
#include "Materials.hpp"

namespace vengine
{

void Material::setTransparent(bool transparent)
{
    m_materials.materialTransparencyChanged(this);
    return;
}

void MaterialPBRStandard::setAlbedoTexture(Texture *texture)
{
    m_albedoTexture = texture;
}

void MaterialPBRStandard::setMetallicTexture(Texture *texture)
{
    m_metallicTexture = texture;
}

void MaterialPBRStandard::setRoughnessTexture(Texture *texture)
{
    m_roughnessTexture = texture;
}

void MaterialPBRStandard::setAOTexture(Texture *texture)
{
    m_aoTexture = texture;
}

void MaterialPBRStandard::setEmissiveTexture(Texture *texture)
{
    m_emissiveTexture = texture;
}

void MaterialPBRStandard::setNormalTexture(Texture *texture)
{
    m_normalTexture = texture;
}

void MaterialPBRStandard::setAlphaTexture(Texture *texture)
{
    m_alphaTexture = texture;
}

Texture *MaterialPBRStandard::getAlbedoTexture() const
{
    return m_albedoTexture;
}

Texture *MaterialPBRStandard::getMetallicTexture() const
{
    return m_metallicTexture;
}

Texture *MaterialPBRStandard::getRoughnessTexture() const
{
    return m_roughnessTexture;
}

Texture *MaterialPBRStandard::getAOTexture() const
{
    return m_aoTexture;
}

Texture *MaterialPBRStandard::getEmissiveTexture() const
{
    return m_emissiveTexture;
}

Texture *MaterialPBRStandard::getNormalTexture() const
{
    return m_normalTexture;
}

Texture *MaterialPBRStandard::getAlphaTexture() const
{
    return m_alphaTexture;
}

bool MaterialPBRStandard::isEmissive() const
{
    return (emissiveIntensity() > std::numeric_limits<float>::epsilon()) && !isBlack(emissiveColor(), 0.01F);
}

void MaterialLambert::setAlbedoTexture(Texture *texture)
{
    m_albedoTexture = texture;
}

void MaterialLambert::setAOTexture(Texture *texture)
{
    m_aoTexture = texture;
}

void MaterialLambert::setEmissiveTexture(Texture *texture)
{
    m_emissiveTexture = texture;
}

void MaterialLambert::setNormalTexture(Texture *texture)
{
    m_normalTexture = texture;
}

void MaterialLambert::setAlphaTexture(Texture *texture)
{
    m_alphaTexture = texture;
}

Texture *MaterialLambert::getAlbedoTexture() const
{
    return m_albedoTexture;
}

Texture *MaterialLambert::getAOTexture() const
{
    return m_aoTexture;
}

Texture *MaterialLambert::getEmissiveTexture() const
{
    return m_emissiveTexture;
}

Texture *MaterialLambert::getNormalTexture() const
{
    return m_normalTexture;
}

Texture *MaterialLambert::getAlphaTexture() const
{
    return m_alphaTexture;
}

bool MaterialLambert::isEmissive() const
{
    return (emissiveIntensity() > std::numeric_limits<float>::epsilon()) && !isBlack(emissiveColor(), 0.01F);
}

void MaterialSkybox::setMap(EnvironmentMap *envMap)
{
    m_envMap = envMap;
}

EnvironmentMap *MaterialSkybox::getMap() const
{
    return m_envMap;
}

}  // namespace vengine