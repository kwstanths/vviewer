#include "WidgetMaterialPicker.hpp"

#include "qgroupbox.h"
#include "qlayout.h"

#include "UI/UIUtils.hpp"

#include <vengine/core/AssetManager.hpp>

#include "WidgetMaterialPBR.hpp"
#include "WidgetMaterialLambert.hpp"
#include "WidgetMaterialVolume.hpp"

using namespace vengine;

WidgetMaterialPicker::WidgetMaterialPicker(QWidget *parent, MaterialType materialType, QString groupBoxName)
    : QWidget(parent)
    , m_materialType(materialType)
{
    m_comboBoxAvailableMaterials = new QComboBox();
    updateAvailableMaterials();
    connect(m_comboBoxAvailableMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onMaterialChanged(int)));

    m_widgetGroupBox = new QGroupBox(groupBoxName);
    createUI(nullptr);

    m_layoutMain = new QVBoxLayout();
    m_layoutMain->addWidget(m_widgetGroupBox);
    m_layoutMain->setContentsMargins(0, 0, 0, 0);
    m_layoutMain->setAlignment(Qt::AlignTop);

    setLayout(m_layoutMain);
}

void WidgetMaterialPicker::updateAvailableMaterials()
{
    QStringList availableMaterials = getCreatedMaterials({m_materialType});
    availableMaterials.push_front("");

    m_comboBoxAvailableMaterials->blockSignals(true);
    m_comboBoxAvailableMaterials->clear();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->blockSignals(false);
    if (m_material != nullptr)
        m_comboBoxAvailableMaterials->setCurrentText(QString::fromStdString(m_material->name()));
    else
        m_comboBoxAvailableMaterials->setCurrentText("");
}

void WidgetMaterialPicker::setMaterial(vengine::Material *material)
{
    m_material = material;
    updateAvailableMaterials();
    createUI(createMaterialWidget(material));
}

int WidgetMaterialPicker::getWidgetHeight() const
{
    int height = 50;
    if (m_material == nullptr)
        return height;

    switch (m_material->type()) {
        case MaterialType::MATERIAL_PBR_STANDARD:
            height += WidgetMaterialPBR::HEIGHT;
            break;
        case MaterialType::MATERIAL_LAMBERT:
            height += WidgetMaterialLambert::HEIGHT;
            break;
        case MaterialType::MATERIAL_VOLUME:
            height += WidgetMaterialVolume::HEIGHT;
            break;
        default:
            break;
    }
    return height;
}

QWidget *WidgetMaterialPicker::createMaterialWidget(Material *material)
{
    if (m_materialWidget != nullptr) {
        delete m_materialWidget;
        m_materialWidget = nullptr;
    }

    if (!material) {
        m_materialWidget = nullptr;
        return nullptr;
    }

    switch (material->type()) {
        case MaterialType::MATERIAL_PBR_STANDARD:
            m_materialWidget = new WidgetMaterialPBR(this, dynamic_cast<MaterialPBRStandard *>(material));
            break;
        case MaterialType::MATERIAL_LAMBERT:
            m_materialWidget = new WidgetMaterialLambert(this, dynamic_cast<MaterialLambert *>(material));
            break;
        case MaterialType::MATERIAL_VOLUME:
            m_materialWidget = new WidgetMaterialVolume(this, dynamic_cast<MaterialVolume *>(material));
            break;
        default:
            break;
    }
    return m_materialWidget;
}

void WidgetMaterialPicker::createUI(QWidget *widgetMaterial)
{
    if (m_layoutGroupBox != nullptr) {
        delete m_layoutGroupBox;
        m_layoutGroupBox = nullptr;
    }

    m_layoutGroupBox = new QVBoxLayout();
    m_layoutGroupBox->addWidget(m_comboBoxAvailableMaterials);
    if (m_materialWidget != nullptr)
        m_layoutGroupBox->addWidget(m_materialWidget);

    m_layoutGroupBox->setContentsMargins(5, 5, 5, 5);
    m_layoutGroupBox->setSpacing(15);
    m_layoutGroupBox->setAlignment(Qt::AlignTop);

    m_widgetGroupBox->setLayout(m_layoutGroupBox);
}

void WidgetMaterialPicker::onMaterialChanged(int)
{
    std::string newMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();

    Material *newMat = nullptr;
    if (newMaterial.empty()) {
        newMat = nullptr;
    } else {
        auto &materials = AssetManager::getInstance().materialsMap();
        newMat = materials.get(newMaterial);
    }
    m_material = newMat;
    createUI(createMaterialWidget(newMat));
    Q_EMIT materialChanged(newMat);
}
