#include "WidgetLightMaterial.hpp"

#include <memory>

#include <qcolordialog.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include <vengine/core/AssetManager.hpp>
#include <Console.hpp>

#include "UI/UIUtils.hpp"

using namespace vengine;

WidgetLightMaterial::WidgetLightMaterial(QWidget *parent, std::shared_ptr<Light> light)
    : QWidget(parent)
{
    m_light = light;

    /* Create available light materials widget */
    m_widgetLightMaterials = new QComboBox();
    m_widgetLightMaterials->addItems(getCreatedLightMaterials());
    m_widgetLightMaterials->setCurrentText(QString::fromStdString(light->lightMaterial->name()));
    connect(m_widgetLightMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onLightMaterialChanged(int)));

    /* Create color button */
    m_lightColorWidget = new WidgetColorButton(this, light->lightMaterial->color);
    connect(m_lightColorWidget, SIGNAL(colorChanged(glm::vec3)), this, SLOT(onLightColorChanged(glm::vec3)));

    /* Create intensity slider */
    m_lightIntensityWidget = new WidgetSliderValue(this, 0, 100, light->lightMaterial->intensity, 1);
    connect(m_lightIntensityWidget, SIGNAL(valueChanged(double)), this, SLOT(onLightIntensityChanged(double)));
    QHBoxLayout *lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensityWidget);
    lightIntensityLayout->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetLightIntensity = new QWidget();
    widgetLightIntensity->setLayout(lightIntensityLayout);

    /* Create create new material widget*/
    m_buttonCreate = new QPushButton("Create new");
    connect(m_buttonCreate, &QPushButton::released, this, &WidgetLightMaterial::onCreateNewMaterial);

    QGroupBox *lightGroupBox = new QGroupBox("Light material");
    QVBoxLayout *layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_widgetLightMaterials);
    layoutLight->addWidget(m_lightColorWidget);
    layoutLight->addWidget(widgetLightIntensity);
    layoutLight->addWidget(m_buttonCreate);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}

void WidgetLightMaterial::onLightMaterialChanged(int)
{
    std::string newLightMaterial = m_widgetLightMaterials->currentText().toStdString();

    auto &lightMaterials = AssetManager::getInstance().lightMaterialsMap();
    auto lightMaterial = lightMaterials.get(newLightMaterial);

    if (lightMaterial == nullptr) {
        debug_tools::ConsoleWarning("Material: " + newLightMaterial + " doesn't exist");
        return;
    }

    m_light->lightMaterial = lightMaterial;
    m_lightIntensityWidget->setValue(lightMaterial->intensity);
    m_lightColorWidget->setColor(lightMaterial->color);
}

void WidgetLightMaterial::onLightColorChanged(glm::vec3 color)
{
    m_light->lightMaterial->color = color;
}

void WidgetLightMaterial::onLightIntensityChanged(double)
{
    double intensity = m_lightIntensityWidget->getValue();
    m_light->lightMaterial->intensity = intensity;
}

void WidgetLightMaterial::onCreateNewMaterial()
{
    static uint32_t newLightMaterialNameIndex = 0;
    std::string name = "LightMaterial" + std::to_string(newLightMaterialNameIndex);

    auto &lightMaterials = AssetManager::getInstance().lightMaterialsMap();
    auto newLightMaterial = lightMaterials.add(std::make_shared<LightMaterial>(name, glm::vec3(1, 1, 1), 1.F));

    m_widgetLightMaterials->blockSignals(true);

    m_widgetLightMaterials->clear();
    m_widgetLightMaterials->addItems(getCreatedLightMaterials());
    m_widgetLightMaterials->setCurrentText(QString::fromStdString(name));
    m_light->lightMaterial = newLightMaterial;
    m_lightIntensityWidget->setValue(newLightMaterial->intensity);
    m_lightColorWidget->setColor(newLightMaterial->color);

    m_widgetLightMaterials->blockSignals(false);
}
