#ifndef __WidgetLightMaterial_hpp__
#define __WidgetLightMaterial_hpp__

#include <qwidget.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qlabel.h>
#include <qcombobox.h>

#include <core/Lights.hpp>

#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"

/* A UI widget for a light material */
class WidgetLightMaterial : public QWidget
{
    Q_OBJECT
public:
    WidgetLightMaterial(QWidget * parent, Light * light);

private:
    Light * m_light;

    QComboBox * m_widgetLightMaterials;

    WidgetColorButton * m_lightColorWidget;
    WidgetSliderValue * m_lightIntensityWidget;

    QPushButton * m_buttonCreate;
private slots:
    void onLightMaterialChanged(int);
    void onLightColorChanged(glm::vec3);
    void onLightIntensityChanged(double);
    void onCreateNewMaterial();
};

#endif