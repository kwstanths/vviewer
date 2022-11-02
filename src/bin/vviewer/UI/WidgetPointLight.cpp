#include "WidgetPointLight.hpp"

#include <qcolordialog.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include "UIUtils.hpp"

WidgetPointLight::WidgetPointLight(QWidget * parent, PointLight * light) : QWidget(parent)
{
    m_light = light;

    /* Create color button */
    m_lightColorWidget = new WidgetColorButton(this, light->color);
    connect(m_lightColorWidget, SIGNAL(colorChanged(glm::vec3)), this, SLOT(onLightColorChanged(glm::vec3)));

    /* Create intensity slider */
    m_lightIntensityWidget = new WidgetSliderValue(this, 0, 100, m_light->intensity, 1);
    connect(m_lightIntensityWidget, SIGNAL(valueChanged(double)), this, SLOT(onLightIntensityChanged(double)));
    QHBoxLayout* lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensityWidget);
    lightIntensityLayout->setContentsMargins(0, 0, 0, 0);
    QWidget* widgetLightIntensity = new QWidget();
    widgetLightIntensity->setLayout(lightIntensityLayout);

    QGroupBox* lightGroupBox = new QGroupBox("Point light");
    QVBoxLayout* layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_lightColorWidget);
    layoutLight->addWidget(widgetLightIntensity);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}

void WidgetPointLight::onLightColorChanged(glm::vec3 color)
{
    m_light->color = color;
}

void WidgetPointLight::onLightIntensityChanged(double)
{
    double intensity = m_lightIntensityWidget->getValue();
    m_light->intensity = intensity;
}