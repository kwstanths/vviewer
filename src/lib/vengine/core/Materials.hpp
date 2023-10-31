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
    virtual Material *createMaterial(const std::string &name, const std::string &filepath, MaterialType type) = 0;
    virtual Material *createMaterial(const std::string &name, MaterialType type) = 0;
    virtual Material *createMaterialFromDisk(std::string name, std::string stackDirectory, Textures &textures) = 0;
    virtual Material *createZipMaterial(std::string name, std::string filename, Textures &textures) = 0;
    virtual std::vector<Material *> createImportedMaterials(const std::vector<ImportedMaterial> &importedMaterials,
                                                            Textures &textures) = 0;
};

}  // namespace vengine
#endif