#include "WidgetEnvironment.hpp"

#include <qlayout.h>
#include <qlabel.h>

#include <core/AssetManager.hpp>
#include <core/Materials.hpp>

#include "UIUtils.hpp"


WidgetEnvironment::WidgetEnvironment(QWidget* parent) : QWidget(parent) 
{
    m_comboMaps = new QComboBox();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    connect(m_comboMaps, SIGNAL(currentIndexChanged(int)), this, SLOT(onMapChanged(int)));

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel("Environment maps:"));
    layoutMain->addWidget(m_comboMaps);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    layoutMain->setAlignment(Qt::AlignTop);
    setLayout(layoutMain);
    setFixedHeight(60);
}

void WidgetEnvironment::updateMaps()
{
    m_comboMaps->blockSignals(true);
    m_comboMaps->clear();
    m_comboMaps->addItems(getImportedEnvironmentMaps());
    m_comboMaps->blockSignals(false);
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