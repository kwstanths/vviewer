#ifndef __VulkanLight_hpp__
#define __VulkanLight_hpp__

#include "core/Light.hpp"

#include "vulkan/resources/VulkanUBOAccessors.hpp"

namespace vengine
{

class VulkanPointLight : public PointLight, public VulkanUBODefault<LightData>::Block
{
public:
    VulkanPointLight(const AssetInfo &info, VulkanUBODefault<LightData> &lightsUBO);

    glm::vec4 &color() override;
    const glm::vec4 &color() const override;

    LightIndex lightIndex() const override;

private:
};

class VulkanDirectionalLight : public DirectionalLight, public VulkanUBODefault<LightData>::Block
{
public:
    VulkanDirectionalLight(const AssetInfo &info, VulkanUBODefault<LightData> &lightsUBO);

    glm::vec4 &color() override;
    const glm::vec4 &color() const override;

    LightIndex lightIndex() const override;

private:
};

}  // namespace vengine

#endif