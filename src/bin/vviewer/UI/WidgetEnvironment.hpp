#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlabel.h>

#include "core/Materials.hpp"
#include "core/Lights.hpp"
#include "math/Transform.hpp"

#include "WidgetTransform.hpp"

/* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent, std::shared_ptr<DirectionalLight> light);

    void updateMaps();

private:
    QComboBox* m_comboMaps;

    WidgetTransform* m_lightTransform;
    QPushButton* m_lightColorButton;
    QSlider* m_lightIntensitySlider;
    QLabel* m_lightIntensityValue;
    QColor m_lightColor;
    std::shared_ptr<DirectionalLight> m_light;

    void setLightButtonColor();
    void setLightColor(QColor color, float intensity);

private slots:
    void onMapChanged(int);
    void onLightColorButton();
    void onLightDirectionChanged(double);
    void onLightColorChanged(QColor color);
    void onLightIntensityChanged(int);
};

#endif
