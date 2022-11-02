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
#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"

/* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent, Scene* scene);

    void updateMaps();

    void setCamera(std::shared_ptr<Camera> c);

    void setEnvironmentType(const EnvironmentType& type, bool updateUI = false);

    void setDirectionalLight(std::shared_ptr<DirectionalLight> light);

private:
    QComboBox* m_environmentPicker;
    QComboBox* m_comboMaps;
    WidgetColorButton * m_backgroundColorWidget;
    QVBoxLayout* m_layoutEnvironmentGroupBox;

    QSlider* m_exposureSlider;

    /* Directional light widgets */
    WidgetTransform* m_lightTransform;
    WidgetColorButton * m_lightColorWidget;
    WidgetSliderValue * m_lightIntensityWidget;

    /* Camera widgets */
    WidgetTransform* m_cameraTransformWidget;
    bool m_cameraTransformWidgetChanged = false;
    QDoubleSpinBox * m_cameraFov = nullptr;

    Scene* m_scene;
    std::shared_ptr<DirectionalLight> m_light;
    std::shared_ptr<PerspectiveCamera> m_camera;

    /* Update timer */
    QTimer* m_updateTimer;

private slots:
    void onEnvironmentChanged(int);
    void onEnvironmentMapChanged(int);
    void onBackgroundColorChanged(glm::vec3);
    void onLightDirectionChanged(double);
    void onLightColorChanged(glm::vec3);
    void onLightIntensityChanged(double);
    void onExposureChanged(int);
    void onCameraWidgetChanged(double);
    void onCameraFovChanged(double);

    void updateCamera();
};

#endif
