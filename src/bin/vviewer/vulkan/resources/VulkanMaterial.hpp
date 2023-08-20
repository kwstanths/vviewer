#ifndef __VulkanMaterials_hpp__
#define __VulkanMaterials_hpp__

#include <core/Materials.hpp>

#include "vulkan/common/VulkanStructs.hpp"
#include "vulkan/resources/VulkanUBO.hpp"
#include "vulkan/resources/VulkanTexture.hpp"

namespace vengine
{

/* A class to handle the UBO storage for a material */
template <typename T>
class VulkanMaterialStorage
{
public:
    VulkanMaterialStorage(VulkanUBO<T> &materialsUBO)
        : m_materialsDataStorage(materialsUBO)
    {
        /* Get an index for a free block in the material storage buffers */
        m_materialsUBOBlockIndex = static_cast<uint32_t>(m_materialsDataStorage.getFree());
        /* Get pointer to free block */
        m_data = m_materialsDataStorage.block(m_materialsUBOBlockIndex);
    }

    ~VulkanMaterialStorage()
    {
        /* Remove block index from the buffers */
        m_materialsDataStorage.remove(m_materialsUBOBlockIndex);
    }

    uint32_t getUBOBlockIndex() const { return m_materialsUBOBlockIndex; }

    uint32_t getBlockSizeAligned() const { return m_materialsDataStorage.getBlockSizeAligned(); }

protected:
    VulkanUBO<T> &m_materialsDataStorage;
    uint32_t m_materialsUBOBlockIndex = -1;
    T *m_data = nullptr;
};

/* A class that wraps descriptor creation for a material, needs to be implemented by each material */
class VulkanMaterialDescriptor
{
public:
    VulkanMaterialDescriptor(VkDescriptorSetLayout descriptorSetLayout);

    virtual VkResult createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) = 0;
    virtual void updateDescriptorSets(VkDevice device, size_t images) = 0;
    virtual void updateDescriptorSet(VkDevice device, size_t index) = 0;

    VkDescriptorSet getDescriptor(size_t index);
    bool needsUpdate(size_t index) const;

protected:
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<bool> m_descirptorsNeedUpdate;
};

/** MATERIALS **/

/* Default PBR material */
class VulkanMaterialPBRStandard : public MaterialPBRStandard, public VulkanMaterialStorage<MaterialData>
{
public:
    VulkanMaterialPBRStandard(std::string name,
                              glm::vec4 albedo,
                              float metallic,
                              float roughness,
                              float ao,
                              float emissive,
                              VkDevice device,
                              VkDescriptorSetLayout descriptorLayout,
                              VulkanUBO<MaterialData> &materialsUBO);

    MaterialIndex getMaterialIndex() const override;

    glm::vec4 &albedo() override;
    glm::vec4 getAlbedo() const override;
    float &metallic() override;
    float getMetallic() const override;
    float &roughness() override;
    float getRoughness() const override;
    float &ao() override;
    float getAO() const override;
    float &emissive() override;
    float getEmissive() const override;
    float &uTiling() override;
    float getUTiling() const override;
    float &vTiling() override;
    float getVTiling() const override;

    void setAlbedoTexture(std::shared_ptr<Texture> texture) override;
    void setMetallicTexture(std::shared_ptr<Texture> texture) override;
    void setRoughnessTexture(std::shared_ptr<Texture> texture) override;
    void setAOTexture(std::shared_ptr<Texture> texture) override;
    void setEmissiveTexture(std::shared_ptr<Texture> texture) override;
    void setNormalTexture(std::shared_ptr<Texture> texture) override;

private:
};

/* Lambert material */
class VulkanMaterialLambert : public MaterialLambert, public VulkanMaterialStorage<MaterialData>
{
public:
    VulkanMaterialLambert(std::string name,
                          glm::vec4 albedo,
                          float ao,
                          float emissive,
                          VkDevice device,
                          VkDescriptorSetLayout descriptorLayout,
                          VulkanUBO<MaterialData> &materialsUBO);

    MaterialIndex getMaterialIndex() const override;

    glm::vec4 &albedo() override;
    glm::vec4 getAlbedo() const override;
    float &ao() override;
    float getAO() const override;
    float &emissive() override;
    float getEmissive() const override;
    float &uTiling() override;
    float getUTiling() const override;
    float &vTiling() override;
    float getVTiling() const override;

    void setAlbedoTexture(std::shared_ptr<Texture> texture) override;
    void setAOTexture(std::shared_ptr<Texture> texture) override;
    void setEmissiveTexture(std::shared_ptr<Texture> texture) override;
    void setNormalTexture(std::shared_ptr<Texture> texture) override;

private:
};

/* Skybox material */
class VulkanMaterialSkybox : public MaterialSkybox, public VulkanMaterialDescriptor
{
public:
    VulkanMaterialSkybox(std::string name,
                         std::shared_ptr<EnvironmentMap> envMap,
                         VkDevice device,
                         VkDescriptorSetLayout descriptorLayout);

    virtual void setMap(std::shared_ptr<EnvironmentMap> envMap) override;

    VkResult createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) override;
    void updateDescriptorSets(VkDevice device, size_t images) override;
    void updateDescriptorSet(VkDevice device, size_t index) override;

private:
};

}  // namespace vengine

#endif
