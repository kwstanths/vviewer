#include "WidgetPointLight.hpp"

#include <qcolordialog.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include "UIUtils.hpp"

WidgetPointLight::WidgetPointLight(QWidget * parent, PointLight * light) : QWidget(parent)
{
    m_light = light;

    /* Create color button */
    m_colorWidget = createColorWidget(&m_colorButton);
    glm::vec3 lightColor = m_light->color;
    m_lightColor = QColor(lightColor.r * 255, lightColor.g * 255, lightColor.b * 255);
    setButtonColor(m_colorButton, m_lightColor);
    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onLightColorButton()));

    /* Create intensity slider */
    m_lightIntensitySlider = new QSlider(Qt::Horizontal);
    m_lightIntensitySlider->setMinimum(0);
    m_lightIntensitySlider->setMaximum(1000);
    m_lightIntensitySlider->setValue(light->intensity * 100.F);
    connect(m_lightIntensitySlider, SIGNAL(valueChanged(int)), this, SLOT(onLightIntensityChanged(int)));
    m_lightIntensityValue = new QLabel(QString::number(getIntensity()));
    QHBoxLayout* lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensitySlider);
    lightIntensityLayout->addWidget(m_lightIntensityValue);
    lightIntensityLayout->setContentsMargins(0, 0, 0, 0);
    QWidget* widgetLightIntensity = new QWidget();
    widgetLightIntensity->setLayout(lightIntensityLayout);

    QGroupBox* lightGroupBox = new QGroupBox("Point light");
    QVBoxLayout* layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_colorWidget);
    layoutLight->addWidget(widgetLightIntensity);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}

void WidgetPointLight::onLightColorButton()
{
    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onLightColorChanged(QColor)));
    dialog->exec();

    setButtonColor(m_colorButton, m_lightColor);
}

void WidgetPointLight::onLightColorChanged(QColor color)
{
    m_lightColor = color;
    m_light->color = glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f);
}

void WidgetPointLight::onLightIntensityChanged(int)
{
    float intensity = getIntensity();
    m_light->intensity = intensity;
    m_lightIntensityValue->setText(QString::number(intensity));
}

float WidgetPointLight::getIntensity()
{
    return (float)m_lightIntensitySlider->value() / 100.;
}
