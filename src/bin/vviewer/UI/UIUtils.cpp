#include "UIUtils.hpp"

#include <string>

#include <core/AssetManager.hpp>
#include <core/MeshModel.hpp>
#include <core/Materials.hpp>
#include <core/EnvironmentMap.hpp>

QStringList getImportedModels()
{
    QStringList importedModels;
    AssetManager<std::string, MeshModel*>& instance = AssetManager<std::string, MeshModel*>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedModels.push_back(QString::fromStdString(itr->first));
    }
    return importedModels;
}

QStringList getCreatedMaterials()
{
    QStringList createdMaterials;
    AssetManager<std::string, Material*>& instance = AssetManager<std::string, Material*>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        createdMaterials.push_back(QString::fromStdString(itr->first));
    }
    return createdMaterials;
}

QStringList getImportedTextures(TextureType type)
{
    QStringList importedTextures;
    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        if (type == TextureType::NO_TYPE || itr->second->m_type == type) {
            importedTextures.push_back(QString::fromStdString(itr->first));
        }
    }
    return importedTextures;
}

QStringList getImportedCubemaps()
{
    QStringList importedCubemaps;
    AssetManager<std::string, Cubemap*>& instance = AssetManager<std::string, Cubemap*>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedCubemaps.push_back(QString::fromStdString(itr->first));
    }
    return importedCubemaps;
}

QStringList getImportedEnvironmentMaps()
{
    QStringList importedEnvMaps;
    AssetManager<std::string, EnvironmentMap*>& instance = AssetManager<std::string, EnvironmentMap*>::getInstance();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedEnvMaps.push_back(QString::fromStdString(itr->first));
    }
    return importedEnvMaps;
}
