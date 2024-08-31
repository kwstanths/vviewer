#include "WidgetLight.hpp"

#include <qcolordialog.h>
#include <qglobal.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qnamespace.h>

#include "UI/UIUtils.hpp"
#include "vengine/core/Light.hpp"
#include "vengine/core/AssetManager.hpp"

#include "WidgetLightPoint.hpp"
#include "WidgetLightDirectional.hpp"

using namespace vengine;

WidgetLight::WidgetLight(QWidget *parent, vengine::ComponentLight &lightComponent)
    : QWidget(parent)
    , m_lightComponent(lightComponent)
{
    m_labelLight = new QLabel("Light:");
    m_labelLight->setFixedWidth(35);

    m_comboBoxLights = new QComboBox();
    updateAvailableLights();
    connect(m_comboBoxLights, SIGNAL(currentIndexChanged(int)), this, SLOT(onLightChanged(int)));

    m_labelCastShadows = new QLabel("Cast shadows:");
    m_labelCastShadows->setFixedWidth(90);

    m_checkBoxCastShadows = new QCheckBox();
    m_checkBoxCastShadows->setChecked(m_lightComponent.castShadows());
    connect(m_checkBoxCastShadows, SIGNAL(stateChanged(int)), this, SLOT(onCheckBoxCastShadows(int)));

    m_widgetGroupBox = new QGroupBox();
    createUI(createLightWidget(lightComponent.light()));

    m_layoutMain = new QVBoxLayout();
    m_layoutMain->addWidget(m_widgetGroupBox);
    m_layoutMain->setContentsMargins(0, 0, 0, 0);
    m_layoutMain->setAlignment(Qt::AlignTop);

    setLayout(m_layoutMain);
}

void WidgetLight::updateAvailableLights()
{
    QStringList availableLights = getCreatedLights();
    m_comboBoxLights->blockSignals(true);
    m_comboBoxLights->clear();
    m_comboBoxLights->addItems(availableLights);
    m_comboBoxLights->blockSignals(false);
    m_comboBoxLights->setCurrentText(QString::fromStdString(m_lightComponent.light()->name()));
}

void WidgetLight::createUI(QWidget *widgetLight)
{
    if (m_layoutGroupBox != nullptr) {
        delete m_layoutGroupBox;
    }

    QHBoxLayout *layoutComboBox = new QHBoxLayout();
    layoutComboBox->addWidget(m_labelLight);
    layoutComboBox->addWidget(m_comboBoxLights);
    layoutComboBox->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetComboBox = new QWidget();
    widgetComboBox->setLayout(layoutComboBox);

    QHBoxLayout *layoutCastShadows = new QHBoxLayout();
    layoutCastShadows->addWidget(m_labelCastShadows);
    layoutCastShadows->addWidget(m_checkBoxCastShadows);
    layoutCastShadows->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetCastShadows = new QWidget();
    widgetCastShadows->setLayout(layoutCastShadows);

    m_layoutGroupBox = new QVBoxLayout();
    m_layoutGroupBox->addWidget(widgetComboBox);
    m_layoutGroupBox->addWidget(widgetLight);
    m_layoutGroupBox->addWidget(widgetCastShadows);
    m_layoutGroupBox->setContentsMargins(5, 5, 5, 5);
    m_layoutGroupBox->setSpacing(15);
    m_layoutGroupBox->setAlignment(Qt::AlignTop);

    m_widgetGroupBox->setLayout(m_layoutGroupBox);
}

QWidget *WidgetLight::createLightWidget(vengine::Light *light)
{
    if (m_widgetLight != nullptr) {
        delete m_widgetLight;
    }

    if (light->type() == LightType::POINT_LIGHT) {
        m_widgetLight = new WidgetLightPoint(this, dynamic_cast<PointLight *>(light));
    } else if (light->type() == LightType::DIRECTIONAL_LIGHT) {
        m_widgetLight = new WidgetLightDirectional(this, dynamic_cast<DirectionalLight *>(light));
    } else {
        return nullptr;
    }

    return m_widgetLight;
}

void WidgetLight::onLightChanged(int)
{
    QString lightName = m_comboBoxLights->currentText();

    auto &lights = AssetManager::getInstance().lightsMap();
    auto light = lights.get(lightName.toStdString());

    createUI(createLightWidget(light));
    m_lightComponent.setLight(light);
}

void WidgetLight::onCheckBoxCastShadows(int)
{
    m_lightComponent.setCastShadows(m_checkBoxCastShadows->isChecked());
}
