#ifndef __VulkanMaterials_hpp__
#define __VulkanMaterials_hpp__

#include <core/Materials.hpp>

#include "VulkanDynamicUBO.hpp"
#include "VulkanDataStructs.hpp"

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
protected:
    VulkanDynamicUBO<MaterialPBRData>& m_materialsDataStorage;
    int m_materialsUBOBlockIndex = -1;
    T * m_data = nullptr;
};

/* A class that wraps descriptor creation for a material, needs to be implemented by each material */
class VulkanMaterialDescriptor {
public:
    virtual bool createDescriptors(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool, size_t images) = 0;
    virtual bool updateDescriptorSets(VkDevice device, size_t images) = 0;
    virtual bool updateDescriptorSet(VkDevice device, size_t index) = 0;

    VkDescriptorSet getDescriptor(size_t index);
    bool needsUpdate(size_t index) const;
protected:
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<bool> m_descirptorsNeedUpdate;
};


/** MATERIALS **/

/* Default PBR material */
class VulkanMaterialPBR : 
    public MaterialPBR, 
    public VulkanMaterialStorage<MaterialPBRData>,
    public VulkanMaterialDescriptor
{
public:
    VulkanMaterialPBR(std::string name, 
        glm::vec4 albedo, 
        float metallic, 
        float roughness, 
        float ao, 
        float emissive, 
        VkDevice device,
        VulkanDynamicUBO<MaterialPBRData>& materialsDynamicUBO, 
        int materialsUBOBlock);

    glm::vec4& getAlbedo() override;
    float& getMetallic() override;
    float& getRoughness() override;
    float& getAO() override;
    float& getEmissive() override;

    void setAlbedoTexture(Texture * texture) override;
    void setMetallicTexture(Texture * texture) override;
    void setRoughnessTexture(Texture * texture) override;
    void setAOTexture(Texture * texture) override;
    void setEmissiveTexture(Texture * texture) override;
    void setNormalTexture(Texture * texture) override;

    bool createSampler(VkDevice device);
    
    bool createDescriptors(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool, size_t images) override;
    bool updateDescriptorSets(VkDevice device, size_t images) override;
    bool updateDescriptorSet(VkDevice device, size_t index) override;

    VkSampler m_sampler;
private:

};


class VulkanMaterialSkybox : 
    public MaterialSkybox, 
    public VulkanMaterialDescriptor 
{
public:
    VulkanMaterialSkybox(std::string name, Cubemap * cubemap, VkDevice device);

    virtual void setMap(Cubemap * cubemap) override;

    bool createDescriptors(VkDevice device, VkDescriptorSetLayout layout, VkDescriptorPool pool, size_t images) override;
    bool updateDescriptorSets(VkDevice device, size_t images) override;
    bool updateDescriptorSet(VkDevice device, size_t index) override;
private:

};

#endif
