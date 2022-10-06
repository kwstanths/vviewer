#include "WidgetEnvironment.hpp"

#include <iostream>

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qcolordialog.h>

#include <core/AssetManager.hpp>
#include <core/Materials.hpp>

#include "UIUtils.hpp"


WidgetEnvironment::WidgetEnvironment(QWidget* parent, Scene* scene) : QWidget(parent)
{
    m_scene = scene;
    m_light = m_scene->getDirectionalLight();
    m_camera = std::dynamic_pointer_cast<PerspectiveCamera>(m_scene->getCamera());

    /* Initialize camera widget */
    /* Initialize camera trasform */
    m_cameraTransformWidget = new WidgetTransform(nullptr, nullptr, "Transform:");
    m_cameraTransformWidget->setTransform(m_camera->getTransform());
    connect(m_cameraTransformWidget->m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionX, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionY, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionZ, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    /* Initialize camera field of view */
    m_cameraFov = new QDoubleSpinBox(nullptr);
    m_cameraFov->setMinimum(1);
    m_cameraFov->setMaximum(179);
    m_cameraFov->setValue(m_camera->getFoV());
    connect(m_cameraFov, SIGNAL(valueChanged(double)), this, SLOT(onCameraFovChanged(double)));
    QHBoxLayout * layoutCameraFoV = new QHBoxLayout();
    layoutCameraFoV->addWidget(new QLabel("Field of view: "));
    layoutCameraFoV->addWidget(m_cameraFov);
    layoutCameraFoV->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetCameraFoV = new QWidget();
    widgetCameraFoV->setLayout(layoutCameraFoV);
    /* Prepate group box */
    QGroupBox* cameraGroupBox = new QGroupBox("Camera:");
    QVBoxLayout * cameraLayout = new QVBoxLayout();
    cameraLayout->addWidget(m_cameraTransformWidget);
    cameraLayout->addWidget(widgetCameraFoV);
    cameraGroupBox->setLayout(cameraLayout);

    /* Initialize environment maps dropdown list widget */
    QGroupBox* envGroupBox = new QGroupBox("Environment:");
    {
        /* Order must be the same as EnvironmentType */
        m_environmentPicker = new QComboBox();
        m_environmentPicker->addItem("Solid color");
        m_environmentPicker->addItem("Environment map");
        m_environmentPicker->addItem("Solid color with environment map lighting");
        m_environmentPicker->setCurrentIndex(1);
        connect(m_environmentPicker, SIGNAL(currentIndexChanged(int)), this, SLOT(onEnvironmentChanged(int)));

        m_comboMaps = new QComboBox();
        m_comboMaps->addItems(getImportedEnvironmentMaps());
        connect(m_comboMaps, SIGNAL(currentIndexChanged(int)), this, SLOT(onEnvironmentMapChanged(int)));

        m_backgroundColorWidget = createColorWidget(&m_backgroundColorButton);
        glm::vec3 backgroundColor = m_scene->getBackgroundColor();
        m_backgroundColor = QColor(backgroundColor.r * 255, backgroundColor.g * 255, backgroundColor.b * 255);
        setButtonColor(m_backgroundColorButton, m_backgroundColor);
        connect(m_backgroundColorButton, SIGNAL(pressed()), this, SLOT(onBackgroundColorButton()));

        m_layoutEnvironmentGroupBox = new QVBoxLayout();
        m_layoutEnvironmentGroupBox->addWidget(m_environmentPicker);
        m_layoutEnvironmentGroupBox->addWidget(m_comboMaps);
        m_layoutEnvironmentGroupBox->addWidget(m_backgroundColorWidget);
        m_backgroundColorWidget->hide();

        envGroupBox->setLayout(m_layoutEnvironmentGroupBox);
    }

    /* Initialize exposure slider */
    m_exposureSlider = new QSlider(Qt::Horizontal);
    m_exposureSlider->setMinimum(0);
    m_exposureSlider->setMaximum(100);
    m_exposureSlider->setValue(50);
    connect(m_exposureSlider, SIGNAL(valueChanged(int)), this, SLOT(onExposureChanged(int)));
    QVBoxLayout* layoutExposure = new QVBoxLayout();
    layoutExposure->addWidget(new QLabel("Exposure:"));
    layoutExposure->addWidget(m_exposureSlider);
    layoutExposure->setContentsMargins(0, 0, 0, 0);
    layoutExposure->setAlignment(Qt::AlignTop);
    QWidget* widgetExposure = new QWidget();
    widgetExposure->setLayout(layoutExposure);

    /* Initialize directional light widget */
    /* Transform */
    m_lightTransform = new WidgetTransform(nullptr, nullptr);
    m_lightTransform->setTransform(m_light->transform);
    connect(m_lightTransform->m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    connect(m_lightTransform->m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    connect(m_lightTransform->m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    /* Color */
    QWidget* widgetLightColor = createColorWidget(&m_lightColorButton);
    m_lightColor = QColor(m_light->color.r * 255, m_light->color.g * 255, m_light->color.b * 255);
    setButtonColor(m_lightColorButton, m_lightColor);
    connect(m_lightColorButton, SIGNAL(pressed()), this, SLOT(onLightColorButton()));
    /* Intensity slider */
    m_lightIntensitySlider = new QSlider(Qt::Horizontal);
    m_lightIntensitySlider->setMinimum(0);
    m_lightIntensitySlider->setMaximum(1000);
    m_lightIntensitySlider->setValue(m_light->intensity * 100.f);
    connect(m_lightIntensitySlider, SIGNAL(valueChanged(int)), this, SLOT(onLightIntensityChanged(int)));
    m_lightIntensityValue = new QLabel(QString::number(m_light->intensity));
    QHBoxLayout* lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensitySlider);
    lightIntensityLayout->addWidget(m_lightIntensityValue);
    lightIntensityLayout->setContentsMargins(0, 0, 0, 0);
    QWidget* widgetLightIntensity = new QWidget();
    widgetLightIntensity->setLayout(lightIntensityLayout);
    /* Group box widget */
    QGroupBox* lightGroupBox = new QGroupBox("Directional light");
    QVBoxLayout* layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_lightTransform);
    layoutLight->addWidget(widgetLightColor);
    layoutLight->addWidget(widgetLightIntensity);
    lightGroupBox->setLayout(layoutLight);

    /* Complete layout */
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(cameraGroupBox);
    layoutMain->addWidget(envGroupBox);
    layoutMain->addWidget(widgetExposure);
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);

    setLayout(layoutMain);
    //setFixedHeight(300);

    /* Set an update timer for the camera transform widget */
    m_updateTimer = new QTimer();
    m_updateTimer->setInterval(30);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateCamera()));
    m_updateTimer->start();
}

void WidgetEnvironment::updateMaps()
{
    m_comboMaps->blockSignals(true);
    m_comboMaps->clear();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    m_comboMaps->setCurrentText(QString::fromStdString(m_scene->getSkybox()->getMap()->m_name));
    m_comboMaps->blockSignals(false);
}

void WidgetEnvironment::setCamera(std::shared_ptr<Camera> c)
{
    m_camera = std::dynamic_pointer_cast<PerspectiveCamera>(c);
    m_cameraFov->setValue(m_camera->getFoV());
}

void WidgetEnvironment::setEnvironmentType(const EnvironmentType& type, bool updateUI)
{
    if (updateUI){
        m_environmentPicker->setCurrentIndex(static_cast<int>(type));
    }

    switch (type)
    {
    case EnvironmentType::SOLID_COLOR:
        m_scene->setEnvironmentType(EnvironmentType::SOLID_COLOR);
        /* Set the contribution of IBL lighting to 0 */
        m_scene->setAmbientIBL(0);
        m_comboMaps->hide();
        m_backgroundColorWidget->show();
        break;
    case EnvironmentType::HDRI:
        m_scene->setEnvironmentType(EnvironmentType::HDRI);
        m_scene->setAmbientIBL(1);
        m_comboMaps->show();
        m_backgroundColorWidget->hide();
        break;
    case EnvironmentType::SOLID_COLOR_WITH_HDRI_LIGHTING:
        m_scene->setEnvironmentType(EnvironmentType::SOLID_COLOR_WITH_HDRI_LIGHTING);
        m_scene->setAmbientIBL(1);
        m_comboMaps->show();
        m_backgroundColorWidget->show();
        break;
    default:
        break;
    }
}

void WidgetEnvironment::updateCamera()
{
    /* If the transform was changed from the UI, update camera and return */
    if (m_cameraTransformWidgetChanged) {
        m_camera->getTransform() = m_cameraTransformWidget->getTransform();
        m_cameraTransformWidgetChanged = false;
        return;
    }

    /* 
        Else, check if the scene camera changed and update the UI with blocked signals, so as to not trigger
        another widget signal
    */
    if (!(m_cameraTransformWidget->getTransform() == m_camera->getTransform()))
    {
        m_cameraTransformWidget->blockSignals(true);
        m_cameraTransformWidget->setTransform(m_camera->getTransform());
        m_cameraTransformWidget->blockSignals(false);
    }
}

void WidgetEnvironment::setLightColor(QColor color)
{
    m_light->color = glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f);
}

float WidgetEnvironment::getIntensity()
{
    return (float)m_lightIntensitySlider->value() / 100.;
}

void WidgetEnvironment::onLightColorButton()
{
    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onLightColorChanged(QColor)));
    dialog->exec();

    setButtonColor(m_lightColorButton, m_lightColor);
}

void WidgetEnvironment::onLightDirectionChanged(double)
{
    m_light->transform = m_lightTransform->getTransform();
}

void WidgetEnvironment::onLightColorChanged(QColor color)
{
    m_lightColor = color;
    setLightColor(m_lightColor);
}

void WidgetEnvironment::onLightIntensityChanged(int val)
{
    float intensity  = getIntensity();
    m_lightIntensityValue->setText(QString::number(intensity));
    m_light->intensity = intensity;
}

void WidgetEnvironment::onBackgroundColorButton()
{
    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onBackgroundColorChanged(QColor)));
    dialog->exec();

    setButtonColor(m_backgroundColorButton, m_backgroundColor);
}

void WidgetEnvironment::onBackgroundColorChanged(QColor color)
{
    m_backgroundColor = color;
    m_scene->setBackgroundColor(glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f));
}

void WidgetEnvironment::onExposureChanged(int)
{
    float exposure = m_exposureSlider->value() / 10.0f - 5.0f;
    m_scene->setExposure(exposure);
}

void WidgetEnvironment::onCameraWidgetChanged(double)
{
    /* 
        If this is called with the signals blocked, it means that the camera signal change was triggered 
        because we updated the UI manually in updateCamera(), don't register another update
    */
    if (!m_cameraTransformWidget->signalsBlocked())
    {
        m_cameraTransformWidgetChanged = true;
    }
}

void WidgetEnvironment::onEnvironmentChanged(int)
{
    EnvironmentType environmentType = static_cast<EnvironmentType>(m_environmentPicker->currentIndex());
    setEnvironmentType(environmentType, false);
}

void WidgetEnvironment::onCameraFovChanged(double)
{
    m_camera->setFoV(m_cameraFov->value());
}

void WidgetEnvironment::onEnvironmentMapChanged(int)
{
    std::string newEnvMap = m_comboMaps->currentText().toStdString();
    
    AssetManager<std::string, EnvironmentMap*>& envMaps = AssetManager<std::string, EnvironmentMap*>::getInstance();
    EnvironmentMap* envMap = envMaps.get(newEnvMap);

    MaterialSkybox* material = m_scene->getSkybox();
    material->setMap(envMap);
}
