#include "UIUtils.hpp"

#include <qlayout.h>
#include <qlabel.h>

#include <string>

#include <vengine/core/AssetManager.hpp>
#include <vengine/core/Model3D.hpp>
#include <vengine/core/Materials.hpp>
#include <vengine/core/EnvironmentMap.hpp>
#include <vengine/core/Light.hpp>

using namespace vengine;

QStringList getImportedModels()
{
    QStringList importedModels;
    auto &instance = AssetManager::getInstance().modelsMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedModels.push_back(QString::fromStdString(itr->first));
    }
    return importedModels;
}

QStringList getCreatedMaterials()
{
    QStringList createdMaterials;
    auto &instance = AssetManager::getInstance().materialsMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        createdMaterials.push_back(QString::fromStdString(itr->first));
    }
    return createdMaterials;
}

QStringList getCreatedMaterials(vengine::MaterialType type)
{
    QStringList createdMaterials;
    auto &instance = AssetManager::getInstance().materialsMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        auto mat = static_cast<Material *>(itr->second);
        if (mat->type() == type) {
            createdMaterials.push_back(QString::fromStdString(itr->first));
        }
    }
    return createdMaterials;
}

QStringList getCreatedLights()
{
    QStringList createdLights;
    auto &instance = AssetManager::getInstance().lightsMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        createdLights.push_back(QString::fromStdString(itr->first));
    }
    return createdLights;
}

QStringList getImportedTextures(ColorSpace colorSpace)
{
    QStringList importedTextures;
    {
        auto &instance = AssetManager::getInstance().texturesMap();
        for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
            auto tex = static_cast<Texture *>(itr->second);
            if (tex->colorSpace() == colorSpace) {
                importedTextures.push_back(QString::fromStdString(itr->first));
            }
        }
    }

    return importedTextures;
}

QStringList getImportedCubemaps()
{
    QStringList importedCubemaps;
    auto &instance = AssetManager::getInstance().cubemapsMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedCubemaps.push_back(QString::fromStdString(itr->first));
    }
    return importedCubemaps;
}

QStringList getImportedEnvironmentMaps()
{
    QStringList importedEnvMaps;
    auto &instance = AssetManager::getInstance().environmentsMapMap();
    for (auto itr = instance.begin(); itr != instance.end(); ++itr) {
        importedEnvMaps.push_back(QString::fromStdString(itr->first));
    }
    return importedEnvMaps;
}

void setButtonColor(QPushButton *button, QColor color)
{
    QString qss = QString("background-color: %1").arg(color.name());
    button->setStyleSheet(qss);
}

void setButtonColor(QPushButton *button, const glm::vec4 &color)
{
    setButtonColor(button, QColor(color.r * 255, color.g * 255, color.b * 255));
}

void setButtonColor(QPushButton *button, const glm::vec3 &color)
{
    setButtonColor(button, QColor(color.r * 255, color.g * 255, color.b * 255));
}

void updateTextureComboBox(QComboBox *box, QStringList names, QString currentText, bool blockSignals)
{
    if (blockSignals) {
        box->blockSignals(true);
    }

    box->clear();
    box->addItems(names);
    box->setCurrentText(currentText);

    if (blockSignals) {
        box->blockSignals(false);
    }
}
