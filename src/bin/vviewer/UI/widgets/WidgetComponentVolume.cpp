#include "WidgetComponentVolume.hpp"

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <vengine/core/AssetManager.hpp>
#include <Console.hpp>
#include <qnamespace.h>

#include "UI/UIUtils.hpp"
#include "WidgetMaterialPBR.hpp"
#include "WidgetMaterialLambert.hpp"

using namespace vengine;

WidgetComponentVolume::WidgetComponentVolume(QWidget *parent, vengine::ComponentVolume &volumeComponent)
    : m_volumeComponent(volumeComponent)
{
    auto materialFrontFacing = volumeComponent.frontFacing();
    auto materialBackFacing = volumeComponent.backFacing();

    m_materialPickerFront = new WidgetMaterialPicker(nullptr, MaterialType::MATERIAL_VOLUME, "Front facing Volume:");
    m_materialPickerBack = new WidgetMaterialPicker(nullptr, MaterialType::MATERIAL_VOLUME, "Back facing Volume:");
    m_materialPickerFront->setMaterial(materialFrontFacing);
    m_materialPickerBack->setMaterial(materialBackFacing);
    connect(m_materialPickerFront,
            SIGNAL(materialChanged(vengine::Material *)),
            this,
            SLOT(onMaterialChangedFrontFacing(vengine::Material *)));
    connect(m_materialPickerBack,
            SIGNAL(materialChanged(vengine::Material *)),
            this,
            SLOT(onMaterialChangedBackFacing(vengine::Material *)));

    m_widgetGroupBox = new QGroupBox();
    createUI();

    m_layoutMain = new QVBoxLayout();
    m_layoutMain->addWidget(m_widgetGroupBox);
    m_layoutMain->setContentsMargins(0, 0, 0, 0);
    m_layoutMain->setAlignment(Qt::AlignTop);

    setLayout(m_layoutMain);
}

void WidgetComponentVolume::updateAvailableMaterials()
{
    m_materialPickerFront->updateAvailableMaterials();
    m_materialPickerBack->updateAvailableMaterials();
}

int WidgetComponentVolume::getWidgetHeight()
{
    int height = 50;
    height += m_materialPickerFront->getWidgetHeight();
    height += m_materialPickerBack->getWidgetHeight();
    return height;
}

void WidgetComponentVolume::createUI()
{
    if (m_layoutGroupBox != nullptr) {
        delete m_layoutGroupBox;
        m_layoutGroupBox = nullptr;
    }

    m_layoutGroupBox = new QVBoxLayout();
    m_layoutGroupBox->addWidget(m_materialPickerFront);
    m_layoutGroupBox->addWidget(m_materialPickerBack);

    m_layoutGroupBox->setContentsMargins(5, 5, 5, 5);
    m_layoutGroupBox->setSpacing(0);
    m_layoutGroupBox->setAlignment(Qt::AlignTop);

    m_widgetGroupBox->setLayout(m_layoutGroupBox);
}

void WidgetComponentVolume::onMaterialChangedFrontFacing(vengine::Material *material)
{
    if (material != nullptr) {
        if (material->type() != MaterialType::MATERIAL_VOLUME)
            return;

        MaterialVolume *materialVolume = static_cast<MaterialVolume *>(material);
        m_volumeComponent.setFrontFacingVolume(materialVolume);
    } else
        m_volumeComponent.setFrontFacingVolume(nullptr);
}

void WidgetComponentVolume::onMaterialChangedBackFacing(vengine::Material *material)
{
    if (material != nullptr) {
        if (material->type() != MaterialType::MATERIAL_VOLUME)
            return;
        MaterialVolume *materialVolume = static_cast<MaterialVolume *>(material);
        m_volumeComponent.setBackFacingVolume(materialVolume);
    } else
        m_volumeComponent.setBackFacingVolume(nullptr);
}