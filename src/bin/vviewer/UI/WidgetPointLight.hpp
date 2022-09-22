#ifndef __WidgetPointLight_hpp__
#define __WidgetPointLight_hpp__

#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>

#include <core/Lights.hpp>

/* A UI widget for a point light */
class WidgetPointLight : public QWidget
{
    Q_OBJECT
public:
    WidgetPointLight(QWidget * parent, PointLight * light);

private:
    PointLight * m_light;

    QPushButton * m_colorButton;
    QWidget * m_colorWidget;
    QColor m_lightColor;

    QSlider* m_lightIntensitySlider;
    QLabel* m_lightIntensityValue;

    float getIntensity();

private slots:
    void onLightColorButton();
    void onLightColorChanged(QColor color);
    void onLightIntensityChanged(int);
};

#endif