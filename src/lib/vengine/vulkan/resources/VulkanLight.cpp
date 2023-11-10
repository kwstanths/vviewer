#include "VulkanLight.hpp"

namespace vengine
{

VulkanPointLight::VulkanPointLight(const std::string &name, VulkanUBO<LightData> &lightsUBO)
    : PointLight(name)
    , VulkanUBOBlock<LightData>(lightsUBO)
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

LightIndex VulkanPointLight::getLightIndex() const
{
    return UBOBlockIndex();
}

VulkanDirectionalLight::VulkanDirectionalLight(const std::string &name, VulkanUBO<LightData> &lightsUBO)
    : DirectionalLight(name)
    , VulkanUBOBlock<LightData>(lightsUBO)
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

LightIndex VulkanDirectionalLight::getLightIndex() const
{
    return UBOBlockIndex();
}

}  // namespace vengine