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
    WidgetPointLight(QWidget * parent, std::shared_ptr<PointLight> light);

private:
    std::shared_ptr<PointLight> m_light;

    WidgetLightMaterial * m_widgetLightMaterial;
};

#endif