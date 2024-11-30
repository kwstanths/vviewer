#ifndef __AssetManager_hpp__
#define __AssetManager_hpp__

#include <unordered_map>
#include <memory>
#include <stdexcept>

#include "Console.hpp"

#include "Asset.hpp"
#include "Texture.hpp"
#include "Image.hpp"
#include "Material.hpp"
#include "Cubemap.hpp"
#include "Model3D.hpp"
#include "EnvironmentMap.hpp"
#include "Light.hpp"

namespace vengine
{

template <typename CastType>
class AssetMap
{
public:
    typedef typename std::unordered_map<std::string, Asset *>::iterator Iterator;

    AssetMap() {}
    AssetMap(AssetMap const &) = delete;
    void operator=(AssetMap const &) = delete;

    bool has(const std::string &id)
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

        debug_tools::ConsoleWarning("Find: Asset not found: " + id);
        return nullptr;
    }

    Iterator remove(const std::string name)
    {
        auto itr = m_assets.find(name);
        if (itr == m_assets.end()) {
            debug_tools::ConsoleWarning("Erase: Asset not found: " + name);
            return m_assets.end();
        }

        return m_assets.erase(itr);
    }

    Iterator remove(const Asset *asset)
    {
        const std::string name = asset->name();
        return remove(name);
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
    AssetMap<Material> &materialsMap() { return m_materialsMap; }
    AssetMap<MaterialSkybox> &materialsSkyboxMap() { return m_materialsSkyboxMap; }
    AssetMap<Light> &lightsMap() { return m_lightsMap; }
    AssetMap<Cubemap> &cubemapsMap() { return m_cubemapsMap; }
    AssetMap<Model3D> &modelsMap() { return m_modelsMap; }
    AssetMap<EnvironmentMap> &environmentsMapMap() { return m_environmentMapsMap; }
    AssetMap<Mesh> &meshesMap() { return m_meshesMap; }

private:
    AssetManager() {}

    AssetMap<Texture> m_texturesMap;
    AssetMap<Material> m_materialsMap;
    AssetMap<MaterialSkybox> m_materialsSkyboxMap;
    AssetMap<Light> m_lightsMap;
    AssetMap<Cubemap> m_cubemapsMap;
    AssetMap<Model3D> m_modelsMap;
    AssetMap<EnvironmentMap> m_environmentMapsMap;
    AssetMap<Mesh> m_meshesMap;
};

}  // namespace vengine

#endif
