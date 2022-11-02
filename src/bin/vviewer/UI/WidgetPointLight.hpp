#ifndef __WidgetPointLight_hpp__
#define __WidgetPointLight_hpp__

#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>

#include <core/Lights.hpp>

#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"

/* A UI widget for a point light */
class WidgetPointLight : public QWidget
{
    Q_OBJECT
public:
    WidgetPointLight(QWidget * parent, PointLight * light);

private:
    PointLight * m_light;

    WidgetColorButton * m_lightColorWidget;
    WidgetSliderValue * m_lightIntensityWidget;

    float getIntensity();

private slots:
    void onLightColorChanged(glm::vec3);
    void onLightIntensityChanged(double);
};

#endif