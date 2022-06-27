#include "WidgetEnvironment.hpp"

#include <iostream>

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qcolordialog.h>

#include <core/AssetManager.hpp>
#include <core/Materials.hpp>

#include "UIUtils.hpp"


WidgetEnvironment::WidgetEnvironment(QWidget* parent, Scene * scene) : QWidget(parent)
{
    m_scene = scene;
    m_light = m_scene->getDirectionalLight();
    m_camera = m_scene->getCamera();

    /* Initialize camera widget */
    m_cameraTransformWidget = new WidgetTransform(nullptr, nullptr, "Camera transform:");
    m_cameraTransformWidget->setTransform(m_camera->getTransform());
    connect(m_cameraTransformWidget->m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionX, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionY, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));
    connect(m_cameraTransformWidget->m_positionZ, SIGNAL(valueChanged(double)), this, SLOT(onCameraWidgetChanged(double)));

    /* Initialize environment maps dropdown list widget */
    m_comboMaps = new QComboBox();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    connect(m_comboMaps, SIGNAL(currentIndexChanged(int)), this, SLOT(onMapChanged(int)));
    QVBoxLayout * layoutMaps = new QVBoxLayout();
    layoutMaps->addWidget(new QLabel("Environment maps:"));
    layoutMaps->addWidget(m_comboMaps);
    layoutMaps->setContentsMargins(0, 0, 0, 0);
    layoutMaps->setAlignment(Qt::AlignTop);
    QWidget* widgetMaps = new QWidget();
    widgetMaps->setLayout(layoutMaps);

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
    m_lightColorButton = new QPushButton();
    m_lightColorButton->setFixedWidth(25);
    m_lightColor = QColor(m_light->color.r * 255, m_light->color.g * 255, m_light->color.b * 255);
    setLightButtonColor();
    connect(m_lightColorButton, SIGNAL(pressed()), this, SLOT(onLightColorButton()));
    QHBoxLayout* lightColorLayout = new QHBoxLayout();
    lightColorLayout->addWidget(new QLabel("Color: "));
    lightColorLayout->addWidget(m_lightColorButton);
    QWidget* widgetLightColor = new QWidget();
    widgetLightColor->setLayout(lightColorLayout);
    /* Intensity slider */
    m_lightIntensitySlider = new QSlider(Qt::Horizontal);
    m_lightIntensitySlider->setMinimum(0);
    m_lightIntensitySlider->setMaximum(1000);
    m_lightIntensitySlider->setValue(100);
    connect(m_lightIntensitySlider, SIGNAL(valueChanged(int)), this, SLOT(onLightIntensityChanged(int)));
    m_lightIntensityValue = new QLabel("1");
    QHBoxLayout* lightIntensityLayout = new QHBoxLayout();
    lightIntensityLayout->addWidget(new QLabel("Intensity:"));
    lightIntensityLayout->addWidget(m_lightIntensitySlider);
    lightIntensityLayout->addWidget(m_lightIntensityValue);
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
    layoutMain->addWidget(m_cameraTransformWidget);
    layoutMain->addWidget(widgetMaps);
    layoutMain->addWidget(widgetExposure);
    layoutMain->addWidget(lightGroupBox);
    layoutMain->setAlignment(Qt::AlignTop);

    setLayout(layoutMain);
    //setFixedHeight(300);

    m_updateTimer = new QTimer();
    m_updateTimer->setInterval(16);
    connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(updateCamera()));
    m_updateTimer->start();
}

void WidgetEnvironment::updateMaps()
{
    m_comboMaps->blockSignals(true);
    m_comboMaps->clear();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    m_comboMaps->blockSignals(false);
}

void WidgetEnvironment::updateCamera()
{
    /* If the transform was changed from the UI, update camera and return */
    if (m_cameraTransformWidgetChanged) {
        m_camera->getTransform() = m_cameraTransformWidget->getTransform();
        m_cameraTransformWidgetChanged = false;
        return;
    }

    /* Else, update the UI itself, block signals to not trigger position change for the camera again */
    m_cameraTransformWidget->blockSignals(true);
    m_cameraTransformWidget->setTransform(m_camera->getTransform());
    m_cameraTransformWidget->blockSignals(false);
}

void WidgetEnvironment::setLightButtonColor()
{
    QString qss = QString("background-color: %1").arg(m_lightColor.name());
    m_lightColorButton->setStyleSheet(qss);
}

void WidgetEnvironment::setLightColor(QColor color, float intensity)
{
    m_light->color = intensity * glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f);
}

void WidgetEnvironment::onLightColorButton()
{
    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onLightColorChanged(QColor)));
    dialog->exec();

    setLightButtonColor();
}

void WidgetEnvironment::onLightDirectionChanged(double)
{
    m_light->transform = m_lightTransform->getTransform();
}

void WidgetEnvironment::onLightColorChanged(QColor color)
{
    m_lightColor = color;
    setLightColor(m_lightColor, (float)m_lightIntensitySlider->value() / 100.);
}

void WidgetEnvironment::onLightIntensityChanged(int val)
{
    float realValue = (float)val / 100.;
    m_lightIntensityValue->setText(QString::number(realValue));
    setLightColor(m_lightColor, realValue);
}

void WidgetEnvironment::onExposureChanged(int)
{
    float exposure = m_exposureSlider->value() / 10.0f - 5.0f;
    m_scene->setExposure(exposure);
}

void WidgetEnvironment::onCameraWidgetChanged(double)
{
    /* Only set the transform to change if the signals haven't been blocked for the widget */
    if (!m_cameraTransformWidget->signalsBlocked())
    {
        m_cameraTransformWidgetChanged = true;
    }
}

void WidgetEnvironment::onMapChanged(int) 
{
    std::string newEnvMap = m_comboMaps->currentText().toStdString();
    
    AssetManager<std::string, EnvironmentMap*>& envMaps = AssetManager<std::string, EnvironmentMap*>::getInstance();
    EnvironmentMap* envMap = envMaps.Get(newEnvMap);

    AssetManager<std::string, MaterialSkybox*>& materials = AssetManager<std::string, MaterialSkybox*>::getInstance();
    MaterialSkybox* material = materials.Get("skybox");

    material->setMap(envMap);
}
