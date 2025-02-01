#ifndef __Materials_hpp__
#define __Materials_hpp__

#include <string>
#include <vector>

#include "io/ImportTypes.hpp"
#include "Material.hpp"
#include "Textures.hpp"

namespace vengine
{

class Engine;

class Materials
{
    friend Material;

public:
    Materials(Engine &engine)
        : m_engine(engine){};

    virtual Material *createMaterial(const AssetInfo &info, MaterialType type) = 0;
    virtual Material *createMaterialFromDisk(const AssetInfo &info, std::string stackDirectory, Textures &textures) = 0;
    virtual std::vector<Material *> createImportedMaterials(const std::vector<ImportedMaterial> &importedMaterials,
                                                            Textures &textures) = 0;

    template <typename MatType>
    MatType *createMaterial(const AssetInfo &info);

protected:
    Engine &m_engine;

private:
    /* Called when a material is set as transparent */
    virtual void materialTransparencyChanged(Material *material);
};

}  // namespace vengine
#endif