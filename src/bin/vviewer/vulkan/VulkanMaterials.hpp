#ifndef __VulkanMaterials_hpp__
#define __VulkanMaterials_hpp__

#include <core/Materials.hpp>

#include "VulkanDynamicUBO.hpp"
#include "VulkanDataStructs.hpp"

/* A class to handle the dynamic UBO storage of a material */
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


class VulkanMaterialPBR : public MaterialPBR, public VulkanMaterialStorage<MaterialPBRData> {
public:
    VulkanMaterialPBR(glm::vec4 albedo, float metallic, float roughness, float ao, float emissive, VulkanDynamicUBO<MaterialPBRData>& materialsDynamicUBO, int materialsUBOBlock);

    glm::vec4& getAlbedo() override;

    float& getMetallic() override;

    float& getRoughness() override;

    float& getAO() override;

    float& getEmissive() override;

private:

};

#endif
