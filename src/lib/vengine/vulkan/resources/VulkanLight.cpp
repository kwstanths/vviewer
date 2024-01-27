#include "VulkanLight.hpp"

namespace vengine
{

VulkanPointLight::VulkanPointLight(const AssetInfo &info, VulkanUBODefault<LightData> &lightsUBO)
    : PointLight(info)
    , VulkanUBODefault<LightData>::Block(lightsUBO)
{
    m_block->type.r = static_cast<unsigned int>(LightType::POINT_LIGHT);
}

glm::vec4 &VulkanPointLight::color()
{
    return m_block->color;
}

const glm::vec4 &VulkanPointLight::color() const
{
    return m_block->color;
}

LightIndex VulkanPointLight::lightIndex() const
{
    return UBOBlockIndex();
}

VulkanDirectionalLight::VulkanDirectionalLight(const AssetInfo &info, VulkanUBODefault<LightData> &lightsUBO)
    : DirectionalLight(info)
    , VulkanUBODefault<LightData>::Block(lightsUBO)
{
    m_block->type.r = static_cast<unsigned int>(LightType::DIRECTIONAL_LIGHT);
}

glm::vec4 &VulkanDirectionalLight::color()
{
    return m_block->color;
}

const glm::vec4 &VulkanDirectionalLight::color() const
{
    return m_block->color;
}

LightIndex VulkanDirectionalLight::lightIndex() const
{
    return UBOBlockIndex();
}

}  // namespace vengine