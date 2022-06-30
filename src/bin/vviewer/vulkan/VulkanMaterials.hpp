#ifndef __VulkanMaterials_hpp__
#define __VulkanMaterials_hpp__

#include <core/Materials.hpp>

#include "VulkanDynamicUBO.hpp"
#include "VulkanDataStructs.hpp"
#include "VulkanTexture.hpp"

/* A class to handle the dynamic UBO storage for a material */
template<typename T> 
class VulkanMaterialStorage {
public:
    VulkanMaterialStorage(VulkanDynamicUBO<T>& materialsDynamicUBO, int materialsUBOBlockIndex)
        : m_materialsDataStorage(materialsDynamicUBO), m_materialsUBOBlockIndex(materialsUBOBlockIndex)
    {
        m_data = m_materialsDataStorage.getBlock(m_materialsUBOBlockIndex);
    }

    int getUBOBlockIndex() const {
        return m_materialsUBOBlockIndex;
    }

    uint32_t getBlockSizeAligned() const {
        return m_materialsDataStorage.getBlockSizeAligned();
    }

protected:
    VulkanDynamicUBO<T>& m_materialsDataStorage;
    int m_materialsUBOBlockIndex = -1;
    T * m_data = nullptr;
};

/* A class that wraps descriptor creation for a material, needs to be implemented by each material */
class VulkanMaterialDescriptor {
public:
    virtual bool createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) = 0;
    virtual bool updateDescriptorSets(VkDevice device, size_t images) = 0;
    virtual bool updateDescriptorSet(VkDevice device, size_t index) = 0;

    VkDescriptorSet getDescriptor(size_t index);
    bool needsUpdate(size_t index) const;
protected:
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<bool> m_descirptorsNeedUpdate;
};


/** MATERIALS **/

/* Default PBR material */
class VulkanMaterialPBRStandard : 
    public MaterialPBRStandard,
    public VulkanMaterialStorage<MaterialData>,
    public VulkanMaterialDescriptor
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
        VulkanDynamicUBO<MaterialData>& materialsDynamicUBO,
        uint32_t materialsUBOBlock);

    glm::vec4& albedo() override;
    glm::vec4 getAlbedo() const override;
    float& metallic() override;
    float getMetallic() const override;
    float& roughness() override;
    float getRoughness() const override;
    float& ao() override;
    float getAO() const override;
    float& emissive() override;
    float getEmissive() const override;

    void setAlbedoTexture(Texture * texture) override;
    void setMetallicTexture(Texture * texture) override;
    void setRoughnessTexture(Texture * texture) override;
    void setAOTexture(Texture * texture) override;
    void setEmissiveTexture(Texture * texture) override;
    void setNormalTexture(Texture * texture) override;

    bool createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) override;
    bool updateDescriptorSets(VkDevice device, size_t images) override;
    bool updateDescriptorSet(VkDevice device, size_t index) override;

private:
    VulkanTexture* m_BRDFLUT = nullptr;
};

/* Lambert material */
class VulkanMaterialLambert :
    public MaterialLambert,
    public VulkanMaterialStorage<MaterialData>,
    public VulkanMaterialDescriptor
{
public:
    VulkanMaterialLambert(std::string name,
        glm::vec4 albedo,
        float ao,
        float emissive,
        VkDevice device,
        VkDescriptorSetLayout descriptorLayout,
        VulkanDynamicUBO<MaterialData>& materialsDynamicUBO,
        uint32_t materialsUBOBlock);

    glm::vec4& albedo() override;
    glm::vec4 getAlbedo() const override;
    float& ao() override;
    float getAO() const override;
    float& emissive() override;
    float getEmissive() const override;

    void setAlbedoTexture(Texture* texture) override;
    void setAOTexture(Texture* texture) override;
    void setEmissiveTexture(Texture* texture) override;
    void setNormalTexture(Texture* texture) override;

    bool createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) override;
    bool updateDescriptorSets(VkDevice device, size_t images) override;
    bool updateDescriptorSet(VkDevice device, size_t index) override;

private:
};

/* Skybox material */
class VulkanMaterialSkybox : 
    public MaterialSkybox, 
    public VulkanMaterialDescriptor 
{
public:
    VulkanMaterialSkybox(std::string name, EnvironmentMap * envMap, VkDevice device, VkDescriptorSetLayout descriptorLayout);

    virtual void setMap(EnvironmentMap * envMap) override;

    bool createDescriptors(VkDevice device, VkDescriptorPool pool, size_t images) override;
    bool updateDescriptorSets(VkDevice device, size_t images) override;
    bool updateDescriptorSet(VkDevice device, size_t index) override;
private:

};

#endif
