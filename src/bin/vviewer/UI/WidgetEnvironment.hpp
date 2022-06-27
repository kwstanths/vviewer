#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qtimer.h>

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

    /* Directional light widgets */
    WidgetTransform* m_lightTransform;
    QPushButton* m_lightColorButton;
    QSlider* m_lightIntensitySlider;
    QLabel* m_lightIntensityValue;
    QColor m_lightColor;

    /* Camera widgets */
    WidgetTransform* m_cameraTransformWidget;
    bool m_cameraTransformWidgetChanged = false;

    Scene* m_scene;
    std::shared_ptr<DirectionalLight> m_light;
    std::shared_ptr<Camera> m_camera;

    void setLightButtonColor();
    void setLightColor(QColor color, float intensity);

    /* Update timer */
    QTimer* m_updateTimer;

private slots:
    void onMapChanged(int);
    void onLightColorButton();
    void onLightDirectionChanged(double);
    void onLightColorChanged(QColor color);
    void onLightIntensityChanged(int);
    void onExposureChanged(int);
    void onCameraWidgetChanged(double);

    void updateCamera();
};

#endif
