#ifndef __WidgetLightPoint_hpp__
#define __WidgetLightPoint_hpp__

#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcombobox.h>

#include <vengine/core/Light.hpp>

#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"

/* A UI widget for a point light */
class WidgetLightPoint : public QWidget
{
    Q_OBJECT
public:
    WidgetLightPoint(QWidget *parent, vengine::Light *light);

private:
    vengine::PointLight *m_light;

    QLabel *m_labelLightType;
    WidgetColorButton *m_lightColorWidget;
    WidgetSliderValue *m_lightIntensityWidget;

private Q_SLOTS:
    void onLightColorChanged(glm::vec3);
    void onLightIntensityChanged(double);
};

#endif