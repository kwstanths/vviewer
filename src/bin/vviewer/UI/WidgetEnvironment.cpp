#include "WidgetEnvironment.hpp"

#include <qlayout.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qcolordialog.h>

#include <core/AssetManager.hpp>
#include <core/Materials.hpp>

#include "UIUtils.hpp"


WidgetEnvironment::WidgetEnvironment(QWidget* parent, std::shared_ptr<DirectionalLight> light) : QWidget(parent)
{
    m_light = light;


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

    m_lightTransform = new WidgetTransform(nullptr, nullptr);
    m_lightTransform->setTransform(m_light->transform);
    connect(m_lightTransform->m_rotationX, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    connect(m_lightTransform->m_rotationY, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    connect(m_lightTransform->m_rotationZ, SIGNAL(valueChanged(double)), this, SLOT(onLightDirectionChanged(double)));
    m_lightColorButton = new QPushButton();
    m_lightColorButton->setFixedWidth(25);
    setLightButtonColor();
    connect(m_lightColorButton, SIGNAL(pressed()), this, SLOT(onLightColorButton()));

    QHBoxLayout* lightColorLayout = new QHBoxLayout();
    lightColorLayout->addWidget(new QLabel("Color: "));
    lightColorLayout->addWidget(m_lightColorButton);
    QWidget* widgetLightColor = new QWidget();
    widgetLightColor->setLayout(lightColorLayout);

    QGroupBox* lightGroupBox = new QGroupBox("Directional light");
    QVBoxLayout* layoutLight = new QVBoxLayout();
    layoutLight->addWidget(m_lightTransform);
    layoutLight->addWidget(widgetLightColor);
    lightGroupBox->setLayout(layoutLight);

    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(widgetMaps);
    layoutMain->addWidget(lightGroupBox);

    setLayout(layoutMain);
    setFixedHeight(280);
}

void WidgetEnvironment::updateMaps()
{
    m_comboMaps->blockSignals(true);
    m_comboMaps->clear();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    m_comboMaps->blockSignals(false);
}

void WidgetEnvironment::setLightButtonColor()
{
    glm::vec3 currentColor = m_light->color;
    QPalette pal = m_lightColorButton->palette();
    pal.setColor(QPalette::Button, QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255));
    m_lightColorButton->setAutoFillBackground(true);
    m_lightColorButton->setPalette(pal);
    m_lightColorButton->update();
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
    m_light->color = glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f);
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
