#include "WidgetPointLight.hpp"

#include <qcolordialog.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include "UIUtils.hpp"

WidgetPointLight::WidgetPointLight(QWidget * parent, ComponentPointLight& lightComponent) : QWidget(parent), m_lightComponent(lightComponent)
{
    m_widgetLightMaterial = new WidgetLightMaterial(this, m_lightComponent.light);

    QGroupBox* lightGroupBox = new QGroupBox("Point Light");
    QVBoxLayout* layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_widgetLightMaterial);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}
