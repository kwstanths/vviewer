#include "WidgetLight.hpp"

#include <qcolordialog.h>
#include <qglobal.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qnamespace.h>

#include "UI/UIUtils.hpp"
#include "vengine/core/Lights.hpp"

using namespace vengine;

WidgetLight::WidgetLight(QWidget *parent, vengine::ComponentLight &lightComponent)
    : QWidget(parent)
    , m_lightComponent(lightComponent)
{
    m_widgetLightMaterial = new WidgetLightMaterial(this, m_lightComponent.light);

    m_lightTypeComboBox = new QComboBox();
    m_lightTypeComboBox->addItem("Point light");
    m_lightTypeComboBox->addItem("Directional light");

    if (lightComponent.light->type == LightType::POINT_LIGHT)
        m_lightTypeComboBox->setCurrentIndex(0);
    else if (lightComponent.light->type == LightType::DIRECTIONAL_LIGHT)
        m_lightTypeComboBox->setCurrentIndex(1);
    connect(m_lightTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onLightTypeChanged(int)));

    QGroupBox *lightGroupBox = new QGroupBox();
    QVBoxLayout *layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_lightTypeComboBox);
    layoutLight->addWidget(m_widgetLightMaterial);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}

void WidgetLight::onLightTypeChanged(int)
{
    if (m_lightTypeComboBox->currentIndex() == 0)
        m_lightComponent.light->type = LightType::POINT_LIGHT;
    else if (m_lightTypeComboBox->currentIndex() == 1)
        m_lightComponent.light->type = LightType::DIRECTIONAL_LIGHT;
}
