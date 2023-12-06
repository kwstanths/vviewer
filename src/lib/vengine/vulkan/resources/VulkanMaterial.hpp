#ifndef __VulkanMaterials_hpp__
#define __VulkanMaterials_hpp__

#include <core/Materials.hpp>

#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanUBO.hpp"
#include "vulkan/resources/VulkanTexture.hpp"

namespace vengine
{

/* A class that wraps descriptor creation for a material, needs to be implemented by each material */
class VulkanMaterialDescriptor
{
public:
    VulkanMaterialDescriptor(VkDescriptorSetLayout descriptorSetLayout);

    virtual VkResult createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) = 0;
    virtual void updateDescriptorSets(VkDevice device, size_t images) = 0;
    virtual void updateDescriptorSet(VkDevice device, size_t index) = 0;

    VkDescriptorSet getDescriptor(size_t index) const;
    bool needsUpdate(size_t index) const;

protected:
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<bool> m_descirptorsNeedUpdate;
};

/** MATERIALS **/

/* Default PBR material */
class VulkanMaterialPBRStandard : public MaterialPBRStandard, public VulkanUBOBlock<MaterialData>
{
public:
    VulkanMaterialPBRStandard(const AssetInfo &info,
                              VkDevice device,
                              VkDescriptorSetLayout descriptorLayout,
                              VulkanUBO<MaterialData> &materialsUBO);

    MaterialIndex materialIndex() const override;

    glm::vec4 &albedo() override;
    const glm::vec4 &albedo() const override;
    float &metallic() override;
    const float &metallic() const override;
    float &roughness() override;
    const float &roughness() const override;
    float &ao() override;
    const float &ao() const override;
    glm::vec4 &emissive() override;
    const glm::vec4 &emissive() const override;
    float &emissiveIntensity() override;
    const float &emissiveIntensity() const override;
    glm::vec3 emissiveColor() const override;
    float &uTiling() override;
    const float &uTiling() const override;
    float &vTiling() override;
    const float &vTiling() const override;

    void setAlbedoTexture(Texture *texture) override;
    void setMetallicTexture(Texture *texture) override;
    void setRoughnessTexture(Texture *texture) override;
    void setAOTexture(Texture *texture) override;
    void setEmissiveTexture(Texture *texture) override;
    void setNormalTexture(Texture *texture) override;
    void setAlphaTexture(Texture *texture) override;

private:
};

/* Lambert material */
class VulkanMaterialLambert : public MaterialLambert, public VulkanUBOBlock<MaterialData>
{
public:
    VulkanMaterialLambert(const AssetInfo &info,
                          VkDevice device,
                          VkDescriptorSetLayout descriptorLayout,
                          VulkanUBO<MaterialData> &materialsUBO);

    MaterialIndex materialIndex() const override;

    glm::vec4 &albedo() override;
    const glm::vec4 &albedo() const override;
    float &ao() override;
    const float &ao() const override;
    glm::vec4 &emissive() override;
    const glm::vec4 &emissive() const override;
    float &emissiveIntensity() override;
    const float &emissiveIntensity() const override;
    glm::vec3 emissiveColor() const override;
    float &uTiling() override;
    const float &uTiling() const override;
    float &vTiling() override;
    const float &vTiling() const override;

    void setAlbedoTexture(Texture *texture) override;
    void setAOTexture(Texture *texture) override;
    void setEmissiveTexture(Texture *texture) override;
    void setNormalTexture(Texture *texture) override;
    void setAlphaTexture(Texture *texture) override;

private:
};

/* Skybox material */
class VulkanMaterialSkybox : public MaterialSkybox, public VulkanMaterialDescriptor
{
public:
    VulkanMaterialSkybox(const AssetInfo &info, EnvironmentMap *envMap, VkDevice device, VkDescriptorSetLayout descriptorLayout);

    virtual void setMap(EnvironmentMap *envMap) override;

    VkResult createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) override;
    void updateDescriptorSets(VkDevice device, size_t images) override;
    void updateDescriptorSet(VkDevice device, size_t index) override;

private:
};

}  // namespace vengine

#endif
