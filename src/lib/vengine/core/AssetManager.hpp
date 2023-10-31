#ifndef __AssetManager_hpp__
#define __AssetManager_hpp__

#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "Asset.hpp"
#include "Texture.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Cubemap.hpp"
#include "Model3D.hpp"
#include "EnvironmentMap.hpp"
#include "Lights.hpp"

namespace vengine
{

template <typename CastType>
class AssetMap
{
public:
    typedef typename std::unordered_map<std::string, Asset *>::iterator Iterator;

    AssetMap() {}

    bool isPresent(const std::string &id)
    {
        if (m_assets.find(id) == m_assets.end()) {
            return false;
        }
        return true;
    }

    CastType *add(Asset *asset)
    {
        m_assets[asset->name()] = asset;
        return static_cast<CastType *>(m_assets[asset->name()]);
    }

    CastType *get(const std::string &id)
    {
        auto itr = m_assets.find(id);
        if (itr != m_assets.end()) {
            return static_cast<CastType *>(itr->second);
        }
        throw std::runtime_error("Find: Asset not found: " + id);
    }

    Iterator remove(const std::string &id)
    {
        auto itr = m_assets.find(id);
        if (itr == m_assets.end()) {
            throw std::runtime_error("Erase: Asset not found: " + id);
        }

        return m_assets.erase(itr);
    }

    void reset() { m_assets.clear(); }

    Iterator begin() { return m_assets.begin(); }

    Iterator end() { return m_assets.end(); }

private:
    std::unordered_map<std::string, Asset *> m_assets;
};

class AssetManager
{
public:
    static AssetManager &getInstance()
    {
        static AssetManager instance;
        return instance;
    }
    AssetManager(AssetManager const &) = delete;
    void operator=(AssetManager const &) = delete;

    AssetMap<Texture> &texturesMap() { return m_texturesMap; }
    AssetMap<Image<stbi_uc>> &imagesCharMap() { return m_imagesCharMap; }
    AssetMap<Image<float>> &imagesHDRMap() { return m_imagesFloatMap; }
    AssetMap<Material> &materialsMap() { return m_materialsMap; }
    AssetMap<MaterialSkybox> &materialsSkyboxMap() { return m_materialsSkyboxMap; }
    AssetMap<LightMaterial> &lightMaterialsMap() { return m_lightMaterialsMap; }
    AssetMap<Cubemap> &cubemapsMap() { return m_cubemapsMap; }
    AssetMap<Model3D> &modelsMap() { return m_modelsMap; }
    AssetMap<EnvironmentMap> &environmentsMapMap() { return m_environmentMapsMap; }

private:
    AssetManager() {}

    AssetMap<Texture> m_texturesMap;
    AssetMap<Image<stbi_uc>> m_imagesCharMap;
    AssetMap<Image<float>> m_imagesFloatMap;
    AssetMap<Material> m_materialsMap;
    AssetMap<MaterialSkybox> m_materialsSkyboxMap;
    AssetMap<LightMaterial> m_lightMaterialsMap;
    AssetMap<Cubemap> m_cubemapsMap;
    AssetMap<Model3D> m_modelsMap;
    AssetMap<EnvironmentMap> m_environmentMapsMap;
};

}  // namespace vengine

#endif
