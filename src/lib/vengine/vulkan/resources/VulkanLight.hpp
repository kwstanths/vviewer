#ifndef __VulkanLight_hpp__
#define __VulkanLight_hpp__

#include "core/Light.hpp"

#include "vulkan/resources/VulkanUBO.hpp"

namespace vengine
{

class VulkanPointLight : public PointLight, public VulkanUBOBlock<LightData>
{
public:
    VulkanPointLight(const AssetInfo &info, VulkanUBO<LightData> &lightsUBO);

    glm::vec4 &color() override;
    const glm::vec4 &color() const override;

    LightIndex lightIndex() const override;

private:
};

class VulkanDirectionalLight : public DirectionalLight, public VulkanUBOBlock<LightData>
{
public:
    VulkanDirectionalLight(const AssetInfo &info, VulkanUBO<LightData> &lightsUBO);

    glm::vec4 &color() override;
    const glm::vec4 &color() const override;

    LightIndex lightIndex() const override;

private:
};

}  // namespace vengine

#endif