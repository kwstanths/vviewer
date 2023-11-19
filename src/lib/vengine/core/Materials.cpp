#include "Materials.hpp"

namespace vengine
{

template <>
MaterialPBRStandard *Materials::createMaterial<MaterialPBRStandard>(const std::string &name)
{
    return static_cast<MaterialPBRStandard *>(createMaterial(name, MaterialType::MATERIAL_PBR_STANDARD));
}

template <>
MaterialLambert *Materials::createMaterial<MaterialLambert>(const std::string &name)
{
    return static_cast<MaterialLambert *>(createMaterial(name, MaterialType::MATERIAL_LAMBERT));
}

}  // namespace vengine