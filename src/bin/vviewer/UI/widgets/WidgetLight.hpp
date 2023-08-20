#ifndef __WidgetPointLight_hpp__
#define __WidgetPointLight_hpp__

#include <qcombobox.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>

#include <core/Lights.hpp>
#include <utils/ECS.hpp>

#include "WidgetLightMaterial.hpp"

/* A UI widget for a point light */
class WidgetLight : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 210;
    WidgetLight(QWidget * parent, vengine::ComponentLight& lightComponent);

private:
    vengine::ComponentLight& m_lightComponent;

    QComboBox * m_lightTypeComboBox;

    WidgetLightMaterial * m_widgetLightMaterial;

private Q_SLOTS:
    void onLightTypeChanged(int);
};

#endif