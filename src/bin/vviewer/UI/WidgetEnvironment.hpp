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
#include "core/Scene.hpp"
#include "math/Transform.hpp"

#include "WidgetTransform.hpp"

/* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent, Scene* scene);

    void updateMaps();

private:
    QComboBox* m_comboMaps;
    QSlider* m_exposureSlider;

    WidgetTransform* m_lightTransform;
    QPushButton* m_lightColorButton;
    QSlider* m_lightIntensitySlider;
    QLabel* m_lightIntensityValue;
    QColor m_lightColor;

    Scene* m_scene;
    std::shared_ptr<DirectionalLight> m_light;

    void setLightButtonColor();
    void setLightColor(QColor color, float intensity);

private slots:
    void onMapChanged(int);
    void onLightColorButton();
    void onLightDirectionChanged(double);
    void onLightColorChanged(QColor color);
    void onLightIntensityChanged(int);
    void onExposureChanged(int);
};

#endif
