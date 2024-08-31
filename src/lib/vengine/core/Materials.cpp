#include "Materials.hpp"
#include "Engine.hpp"

namespace vengine
{

template <>
MaterialPBRStandard *Materials::createMaterial<MaterialPBRStandard>(const AssetInfo &info)
{
    return static_cast<MaterialPBRStandard *>(createMaterial(info, MaterialType::MATERIAL_PBR_STANDARD));
}

template <>
MaterialLambert *Materials::createMaterial<MaterialLambert>(const AssetInfo &info)
{
    return static_cast<MaterialLambert *>(createMaterial(info, MaterialType::MATERIAL_LAMBERT));
}

template <>
MaterialVolume *Materials::createMaterial<MaterialVolume>(const AssetInfo &info)
{
    return static_cast<MaterialVolume *>(createMaterial(info, MaterialType::MATERIAL_VOLUME));
}

void Materials::materialTransparencyChanged(Material *material)
{
    m_engine.scene().invalidateInstances(true);
}

}  // namespace vengine