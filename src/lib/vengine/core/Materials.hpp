#ifndef __Materials_hpp__
#define __Materials_hpp__

#include <string>
#include <vector>

#include "io/ImportTypes.hpp"
#include "Material.hpp"
#include "Textures.hpp"

namespace vengine
{

class Materials
{
public:
    virtual Material *createMaterial(const AssetInfo &info, MaterialType type) = 0;
    virtual Material *createMaterialFromDisk(const AssetInfo &info, std::string stackDirectory, Textures &textures) = 0;
    virtual Material *createZipMaterial(const AssetInfo &info, Textures &textures) = 0;
    virtual std::vector<Material *> createImportedMaterials(const std::vector<ImportedMaterial> &importedMaterials,
                                                            Textures &textures) = 0;

    template <typename MatType>
    MatType *createMaterial(const AssetInfo &info);
};

}  // namespace vengine
#endif