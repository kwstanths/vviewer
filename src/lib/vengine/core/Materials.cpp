#include "Materials.hpp"

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

}  // namespace vengine