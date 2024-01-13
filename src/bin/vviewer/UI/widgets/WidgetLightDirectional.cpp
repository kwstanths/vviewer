#include "WidgetLightDirectional.hpp"

#include <memory>

#include <qcolordialog.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include <vengine/core/AssetManager.hpp>
#include <Console.hpp>

#include "UI/UIUtils.hpp"

using namespace vengine;

WidgetLightDirectional::WidgetLightDirectional(QWidget *parent, Light *light)
    : QWidget(parent)
{
    m_light = static_cast<DirectionalLight *>(light);

    m_labelLightType = new QLabel("Type: Directional light");

    /* Create color button */
    m_lightColorWidget = new WidgetColorButton(this, glm::vec3(m_light->color()));
    connect(m_lightColorWidget, SIGNAL(colorChanged(glm::vec3)), this, SLOT(onLightColorChanged(glm::vec3)));

    /* Create intensity slider */
    m_lightIntensityWidget = new WidgetSliderValue(this, 0, 1000, m_light->color().a, 1);
    connect(m_lightIntensityWidget, SIGNAL(valueChanged(double)), this, SLOT(onLightIntensityChanged(double)));
    QHBoxLayout *lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensityWidget);
    lightIntensityLayout->setContentsMargins(0, 0, 0, 0);
    QWidget *widgetLightIntensity = new QWidget();
    widgetLightIntensity->setLayout(lightIntensityLayout);

    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(m_labelLightType);
    layoutMain->addWidget(m_lightColorWidget);
    layoutMain->addWidget(widgetLightIntensity);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    QWidget *widgetMain = new QWidget();
    widgetMain->setLayout(layoutMain);

    setLayout(layoutMain);
}

void WidgetLightDirectional::onLightColorChanged(glm::vec3 newColor)
{
    m_light->color().r = newColor.r;
    m_light->color().g = newColor.g;
    m_light->color().b = newColor.b;
}

void WidgetLightDirectional::onLightIntensityChanged(double)
{
    double intensity = m_lightIntensityWidget->getValue();
    m_light->color().a = intensity;
}
