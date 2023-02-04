#ifndef __WidgetPointLight_hpp__
#define __WidgetPointLight_hpp__

#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>

#include <core/Lights.hpp>

#include "WidgetLightMaterial.hpp"

/* A UI widget for a point light */
class WidgetPointLight : public QWidget
{
    Q_OBJECT
public:
    WidgetPointLight(QWidget * parent, ComponentPointLight& lightComponent);

private:
    ComponentPointLight& m_lightComponent;

    WidgetLightMaterial * m_widgetLightMaterial;
};

#endif