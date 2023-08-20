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

#include "core/Materials.hpp"
#include "core/Lights.hpp"
#include "core/Scene.hpp"
#include "core/SceneObject.hpp"
#include "math/Transform.hpp"

#include "WidgetTransform.hpp"
#include "WidgetSliderValue.hpp"
#include "WidgetColorButton.hpp"
#include "utils/ECS.hpp"

/* A UI widget to represent environment variables */
class WidgetEnvironment : public QWidget {
    Q_OBJECT
public:
    WidgetEnvironment(QWidget* parent, vengine::Scene* scene);

    void updateMaps();

    void setCamera(std::shared_ptr<vengine::Camera> c);

    void setEnvironmentType(const vengine::EnvironmentType& type, bool updateUI = false);

private:
    QComboBox* m_environmentPicker;
    QComboBox* m_comboMaps;

    WidgetColorButton * m_backgroundColorWidget;
    QVBoxLayout* m_layoutEnvironmentGroupBox;

    QSlider* m_exposureSlider;

    /* Scene info */
    vengine::Scene* m_scene;
    std::shared_ptr<vengine::PerspectiveCamera> m_camera;

    /* Camera widgets */
    WidgetTransform* m_cameraTransformWidget;
    bool m_cameraTransformWidgetChanged = false;
    QDoubleSpinBox * m_cameraFov = nullptr;

    /* Update timer */
    QTimer* m_updateTimer;

    void showEvent(QShowEvent * event) override;

private Q_SLOTS:
    void onEnvironmentChanged(int);
    void onEnvironmentMapChanged(int);
    void onBackgroundColorChanged(glm::vec3);
    void onExposureChanged(int);
    void onCameraWidgetChanged(double);
    void onCameraFovChanged(double);

    void updateCamera();
};

#endif
