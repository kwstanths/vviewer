#include "WidgetMaterial.hpp"

#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include <core/AssetManager.hpp>
#include <Console.hpp>

#include "UIUtils.hpp"
#include "WidgetMaterialPBR.hpp"
#include "WidgetMaterialLambert.hpp"

WidgetMaterial::WidgetMaterial(QWidget* parent, SceneObject* sceneObject)
{
    m_sceneObject = sceneObject;
    Material * material = sceneObject->get<Material*>(ComponentType::MATERIAL);

    QStringList availableMaterials = getCreatedMaterials();

    m_comboBoxAvailableMaterials = new QComboBox();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->setCurrentText(QString::fromStdString(material->m_name));
    connect(m_comboBoxAvailableMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onMaterialChanged(int)));

    m_widgetGroupBox = new QGroupBox(tr("Material"));
    createUI(createMaterialWidget(material));
    
    m_layoutMain = new QVBoxLayout();
    m_layoutMain->addWidget(m_widgetGroupBox);
    m_layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(m_layoutMain);
    //setFixedHeight(600);
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

QWidget* WidgetMaterial::createMaterialWidget(Material* material)
{
    if (m_widgetMaterial != nullptr) {
        delete m_widgetMaterial;
    }

    switch (material->getType())
    {
    case MaterialType::MATERIAL_PBR_STANDARD:
        m_widgetMaterial = new WidgetMaterialPBR(this, dynamic_cast<MaterialPBRStandard*>(material));
        break;
    case MaterialType::MATERIAL_LAMBERT:
        m_widgetMaterial = new WidgetMaterialLambert(this, dynamic_cast<MaterialLambert*>(material));
        break;
    default:
        break;
    }
    return m_widgetMaterial;
}

void WidgetMaterial::onMaterialChanged(int)
{
    std::string newMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();

    AssetManager<std::string, Material*>& instance = AssetManager<std::string, Material*>::getInstance();
    Material* material = instance.Get(newMaterial);

    if (material == nullptr) {
        utils::ConsoleWarning("Material: " + newMaterial + " doesn't exist");
        return;
    }

    createUI(createMaterialWidget(material));
    m_sceneObject->add(material);
}