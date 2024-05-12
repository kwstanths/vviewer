#ifndef __WidgetEnvironment_hpp__
#define __WidgetEnvironment_hpp__

#include <memory>
#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qgroupbox.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/Light.hpp"
#include "vengine/core/Scene.hpp"
#include "vengine/core/SceneObject.hpp"
#include "vengine/math/Transform.hpp"
#include "vengine/utils/ECS.hpp"

#include "WidgetTransform.hpp"
#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"

/* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget
{
    Q_OBJECT
public:
    WidgetEnvironment(QWidget *parent, vengine::Scene *scene);

    void updateMaps();
    void updateMaterials();

    void setCamera(std::shared_ptr<vengine::Camera> c);
    void setCameraVolume(const std::string &name);

    void setEnvironmentType(const vengine::EnvironmentType &type, bool updateUI = false);
    void setBackgroundColor(glm::vec3 backgroundColor);

private:
    QComboBox *m_environmentPicker;
    QComboBox *m_comboMaps;

    WidgetColorButton *m_backgroundColorWidget;
    QVBoxLayout *m_layoutEnvironmentGroupBox;

    QSlider *m_exposureSlider;

    /* Scene info */
    vengine::Scene *m_scene;
    std::shared_ptr<vengine::PerspectiveCamera> m_camera;

    /* Camera widgets */
    WidgetTransform *m_cameraTransformWidget;
    bool m_cameraTransformWidgetChanged = false;
    QDoubleSpinBox *m_cameraFov = nullptr;
    QDoubleSpinBox *m_cameraZFarPlane = nullptr;
    QDoubleSpinBox *m_cameraZNearPlane = nullptr;
    QDoubleSpinBox *m_cameraLensRadius = nullptr;
    QDoubleSpinBox *m_cameraFocalDistance = nullptr;

    QComboBox *m_comboVolumes;

    /* Update timer */
    QTimer *m_updateTimer;

    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
    void onEnvironmentChanged(int);
    void onEnvironmentMapChanged(int);
    void onBackgroundColorChanged(glm::vec3);
    void onExposureChanged(int);
    void onCameraWidgetChanged(double);
    void onCameraFovChanged(double);
    void onCameraZFarChanged(double);
    void onCameraZNearChanged(double);
    void onCameraLensRadiusChanged(double);
    void onCameraFocalDistanceChanged(double);
    void onCameraVolumeChanged(int);

    void updateCamera();
};

#endif
