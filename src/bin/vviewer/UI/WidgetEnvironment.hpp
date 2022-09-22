#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qgroupbox.h>

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

    void setCamera(std::shared_ptr<Camera> c);
private:
    QComboBox* m_environmentPicker;
    QComboBox* m_comboMaps;
    QPushButton* m_backgroundColorButton;
    QColor m_backgroundColor;
    QWidget* m_backgroundColorWidget;
    QVBoxLayout* m_layoutEnvironmentGroupBox;

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
    QDoubleSpinBox * m_cameraFov = nullptr;

    Scene* m_scene;
    std::shared_ptr<DirectionalLight> m_light;
    std::shared_ptr<PerspectiveCamera> m_camera;

    void setLightColor(QColor color);
    float getIntensity();

    /* Update timer */
    QTimer* m_updateTimer;

private slots:
    void onEnvironmentChanged(int);
    void onEnvironmentMapChanged(int);
    void onLightColorButton();
    void onLightDirectionChanged(double);
    void onLightColorChanged(QColor color);
    void onLightIntensityChanged(int);
    void onBackgroundColorButton();
    void onBackgroundColorChanged(QColor color);
    void onExposureChanged(int);
    void onCameraWidgetChanged(double);
    void onCameraFovChanged(double);

    void updateCamera();
};

#endif
