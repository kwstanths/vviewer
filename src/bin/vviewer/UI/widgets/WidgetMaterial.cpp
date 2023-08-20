#include "WidgetMaterial.hpp"

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <core/AssetManager.hpp>
#include <Console.hpp>
#include <qnamespace.h>

#include "UI/UIUtils.hpp"
#include "WidgetMaterialPBR.hpp"
#include "WidgetMaterialLambert.hpp"

using namespace vengine;

WidgetMaterial::WidgetMaterial(QWidget* parent, vengine::ComponentMaterial& materialComponent) : m_materialComponent(materialComponent)
{
    auto material = materialComponent.material;

    m_comboBoxAvailableMaterials = new QComboBox();
    updateAvailableMaterials();
    connect(m_comboBoxAvailableMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onMaterialChanged(int)));

    m_widgetGroupBox = new QGroupBox();
    createUI(createMaterialWidget(material));
    
    m_layoutMain = new QVBoxLayout();
    m_layoutMain->addWidget(m_widgetGroupBox);
    m_layoutMain->setContentsMargins(0, 0, 0, 0);
    m_layoutMain->setAlignment(Qt::AlignTop);

    setLayout(m_layoutMain);
}

void WidgetMaterial::updateAvailableMaterials()
{
    QStringList availableMaterials = getCreatedMaterials();
    m_comboBoxAvailableMaterials->blockSignals(true);
    m_comboBoxAvailableMaterials->clear();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->blockSignals(false);
    m_comboBoxAvailableMaterials->setCurrentText(QString::fromStdString(m_materialComponent.material->m_name));
}

void WidgetMaterial::createUI(QWidget* widgetMaterial)
{
    if (m_layoutGroupBox != nullptr) {
        delete m_layoutGroupBox;
    }

    m_layoutGroupBox = new QVBoxLayout();
    m_layoutGroupBox->addWidget(m_comboBoxAvailableMaterials);
    m_layoutGroupBox->addWidget(widgetMaterial);
    m_layoutGroupBox->setContentsMargins(5, 5, 5, 5);
    m_layoutGroupBox->setSpacing(15);
    m_layoutGroupBox->setAlignment(Qt::AlignTop);

    m_widgetGroupBox->setLayout(m_layoutGroupBox);
}

QWidget* WidgetMaterial::createMaterialWidget(std::shared_ptr<Material>& material)
{
    if (m_widgetMaterial != nullptr) {
        delete m_widgetMaterial;
    }

    switch (material->getType())
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
        m_widgetMaterial = new WidgetMaterialPBR(this, std::dynamic_pointer_cast<MaterialPBRStandard>(material));
        break;
    case MaterialType::MATERIAL_LAMBERT:
        m_widgetMaterial = new WidgetMaterialLambert(this, std::dynamic_pointer_cast<MaterialLambert>(material));
        break;
    default:
        break;
    }
    return m_widgetMaterial;
}

void WidgetMaterial::onMaterialChanged(int)
{
    std::string newMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();

    AssetManager<std::string, Material>& instance = AssetManager<std::string, Material>::getInstance();
    auto material = instance.get(newMaterial);

    if (material == nullptr) {
        debug_tools::ConsoleWarning("Material: " + newMaterial + " doesn't exist");
        return;
    }

    createUI(createMaterialWidget(material));
    m_materialComponent.material = material;
}