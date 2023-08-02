#include "WidgetMaterialPBR.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include "core/AssetManager.hpp"
#include "core/Materials.hpp"

#include "UI/UIUtils.hpp"

WidgetMaterialPBR::WidgetMaterialPBR(QWidget * parent, std::shared_ptr<MaterialPBRStandard> material) : QWidget(parent)
{
    m_material = material;

    QStringList availableColorTextures = getImportedTextures(TextureType::COLOR);
    QStringList availableLinearTextures = getImportedTextures(TextureType::LINEAR);

    m_comboBoxAlbedo = new QComboBox();
    m_comboBoxAlbedo->addItems(availableColorTextures);
    m_comboBoxAlbedo->setCurrentText(QString::fromStdString(material->getAlbedoTexture()->m_name));
    connect(m_comboBoxAlbedo, SIGNAL(currentIndexChanged(int)), this, SLOT(onColorTextureChanged(int)));
    m_colorButton = new QPushButton();
    m_colorButton->setFixedWidth(25);
    setColorButtonColor();
    QGroupBox * groupBoxAlbedo = new QGroupBox(tr("Albedo"));
    QHBoxLayout * layoutAlbedo = new QHBoxLayout();
    layoutAlbedo->addWidget(m_colorButton);
    layoutAlbedo->addWidget(m_comboBoxAlbedo);
    layoutAlbedo->setContentsMargins(5, 5, 5, 5);
    groupBoxAlbedo->setLayout(layoutAlbedo);

    m_comboBoxMetallic = new QComboBox();
    m_comboBoxMetallic->addItems(availableLinearTextures);
    m_comboBoxMetallic->setCurrentText(QString::fromStdString(material->getMetallicTexture()->m_name));
    connect(m_comboBoxMetallic, SIGNAL(currentIndexChanged(int)), this, SLOT(onMetallicTextureChanged(int)));
    m_metallic = new QSlider(Qt::Horizontal);
    m_metallic->setMaximum(100);
    m_metallic->setMinimum(0);
    m_metallic->setSingleStep(1);
    m_metallic->setValue(material->getMetallic() * 100);
    QGroupBox * groupBoxMetallic = new QGroupBox(tr("Metallic"));
    QVBoxLayout * layoutMetallic = new QVBoxLayout();
    layoutMetallic->addWidget(m_metallic);
    layoutMetallic->addWidget(m_comboBoxMetallic);
    layoutMetallic->setContentsMargins(5, 5, 5, 5);
    groupBoxMetallic->setLayout(layoutMetallic);


    m_comboBoxRoughness = new QComboBox();
    m_comboBoxRoughness->addItems(availableLinearTextures);
    m_comboBoxRoughness->setCurrentText(QString::fromStdString(material->getRoughnessTexture()->m_name));
    connect(m_comboBoxRoughness, SIGNAL(currentIndexChanged(int)), this, SLOT(onRoughnessTextureChanged(int)));
    m_roughness = new QSlider(Qt::Horizontal);
    m_roughness->setMaximum(100);
    m_roughness->setMinimum(0);
    m_roughness->setSingleStep(1);
    m_roughness->setValue(material->getRoughness() * 100);
    QGroupBox * groupBoxRoughness = new QGroupBox(tr("Roughness"));
    QVBoxLayout * layoutRoughness = new QVBoxLayout();
    layoutRoughness->addWidget(m_roughness);
    layoutRoughness->addWidget(m_comboBoxRoughness);
    layoutRoughness->setContentsMargins(5, 5, 5, 5);
    groupBoxRoughness->setLayout(layoutRoughness);


    m_comboBoxAO = new QComboBox();
    m_comboBoxAO->addItems(availableLinearTextures);
    m_comboBoxAO->setCurrentText(QString::fromStdString(material->getAOTexture()->m_name));
    connect(m_comboBoxAO, SIGNAL(currentIndexChanged(int)), this, SLOT(onAOTextureChanged(int)));
    m_ao = new QSlider(Qt::Horizontal);
    m_ao->setMaximum(100);
    m_ao->setMinimum(0);
    m_ao->setSingleStep(1);
    m_ao->setValue(material->getAO() * 100);
    QGroupBox * groupBoxAO = new QGroupBox(tr("Ambient"));
    QVBoxLayout * layoutAO = new QVBoxLayout();
    layoutAO->addWidget(m_ao);
    layoutAO->addWidget(m_comboBoxAO);
    layoutAO->setContentsMargins(5, 5, 5, 5);
    groupBoxAO->setLayout(layoutAO);


    m_emissive = new QDoubleSpinBox();
    m_emissive->setMaximum(10000);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->getEmissive());
    QGroupBox * groupBoxEmssive = new QGroupBox(tr("Emissive"));
    QHBoxLayout * layoutEmissive = new QHBoxLayout();
    layoutEmissive->addWidget(new QLabel("Emissive: "));
    layoutEmissive->addWidget(m_emissive);
    layoutEmissive->setContentsMargins(5, 5, 5, 5);
    groupBoxEmssive->setLayout(layoutEmissive);


    m_comboBoxNormal = new QComboBox();
    m_comboBoxNormal->addItems(availableLinearTextures);
    m_comboBoxNormal->setCurrentText(QString::fromStdString(material->getNormalTexture()->m_name));
    connect(m_comboBoxNormal, SIGNAL(currentIndexChanged(int)), this, SLOT(onNormalTextureChanged(int)));
    QGroupBox * groupBoxNormal = new QGroupBox(tr("Normal"));
    QVBoxLayout * layoutNormal = new QVBoxLayout();
    layoutNormal->addWidget(m_comboBoxNormal);
    layoutNormal->setContentsMargins(5, 5, 5, 5);
    groupBoxNormal->setLayout(layoutNormal);


    m_uTiling = new QDoubleSpinBox();
    m_uTiling->setMinimum(0);
    m_uTiling->setMaximum(65536);
    m_uTiling->setValue(material->getUTiling());
    connect(m_uTiling, SIGNAL(valueChanged(double)), this, SLOT(onUTilingChanged(double)));
    m_vTiling = new QDoubleSpinBox();
    m_vTiling->setMinimum(0);
    m_vTiling->setMaximum(65536);
    m_vTiling->setValue(material->getVTiling());
    connect(m_vTiling, SIGNAL(valueChanged(double)), this, SLOT(onVTilingChanged(double)));
    QGroupBox * groupBoxUVTiling = new QGroupBox(tr("UV tiling"));
    QHBoxLayout * layoutUVTiling = new QHBoxLayout();
    layoutUVTiling->addWidget(new QLabel("U:"));
    layoutUVTiling->addWidget(m_uTiling);
    layoutUVTiling->addWidget(new QLabel("V: "));
    layoutUVTiling->addWidget(m_vTiling);
    layoutUVTiling->setContentsMargins(5, 5, 5, 5);
    groupBoxUVTiling->setLayout(layoutUVTiling);

    auto itr = materialTypeNames.find(MaterialType::MATERIAL_PBR_STANDARD);
    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(QString::fromStdString("Type: " + itr->second)));
    layoutMain->addWidget(groupBoxAlbedo);
    layoutMain->addWidget(groupBoxMetallic);
    layoutMain->addWidget(groupBoxRoughness);
    layoutMain->addWidget(groupBoxAO);
    layoutMain->addWidget(groupBoxEmssive);
    layoutMain->addWidget(groupBoxNormal);
    layoutMain->addWidget(groupBoxUVTiling);
    layoutMain->setSpacing(15);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
    setFixedHeight(550);

    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onColorButton()));
    connect(m_metallic, &QSlider::valueChanged, this, &WidgetMaterialPBR::onMetallicChanged);
    connect(m_roughness, &QSlider::valueChanged, this, &WidgetMaterialPBR::onRoughnessChanged);
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialPBR::onAOChanged);
    connect(m_emissive, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialPBR::onEmissiveChanged);

    /* If it'a material parsed from a zip file, disable the UI */
    if (material->m_path != "")
    {
        m_colorButton->setEnabled(false);
        m_metallic->setEnabled(false);
        m_roughness->setEnabled(false);
        m_ao->setEnabled(false);
        m_emissive->setEnabled(false);
        m_comboBoxAlbedo->setEnabled(false);
        m_comboBoxMetallic->setEnabled(false);
        m_comboBoxRoughness->setEnabled(false);
        m_comboBoxAO->setEnabled(false);
        m_comboBoxNormal->setEnabled(false);
        m_uTiling->setEnabled(false);
        m_vTiling->setEnabled(false);
    }
}

void WidgetMaterialPBR::onColorButton()
{
    glm::vec4 currentColor = m_material->getAlbedo();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog * dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);

    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onColorChanged(QColor)));
    dialog->exec();

    /* When the window exits, change the color of the UI button */
    setColorButtonColor();
}

void WidgetMaterialPBR::setColorButtonColor()
{
    glm::vec4 currentColor = m_material->getAlbedo();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);
    QString qss = QString("background-color: %1").arg(col.name());
    m_colorButton->setStyleSheet(qss);

    /*QPalette pal = m_colorButton->palette();
    pal.setColor(QPalette::Button, QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255));
    m_colorButton->setAutoFillBackground(true);
    m_colorButton->setPalette(pal);
    m_colorButton->update();*/
}

void WidgetMaterialPBR::onColorChanged(QColor color) {
    m_material->albedo() = glm::vec4(color.red(), color.green(), color.blue(), color.alpha()) / glm::vec4(255.0f);
}

void WidgetMaterialPBR::onColorTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlbedo->currentText().toStdString();

    AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
    m_material->setAlbedoTexture(instance.get(newTexture));
}

void WidgetMaterialPBR::onMetallicChanged()
{
    m_material->metallic() = static_cast<float>(m_metallic->value()) / 100;
}

void WidgetMaterialPBR::onMetallicTextureChanged(int)
{
    std::string newTexture = m_comboBoxMetallic->currentText().toStdString();

    AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
    m_material->setMetallicTexture(instance.get(newTexture));
}

void WidgetMaterialPBR::onRoughnessChanged()
{
    m_material->roughness() = static_cast<float>(m_roughness->value()) / 100;
}

void WidgetMaterialPBR::onRoughnessTextureChanged(int)
{
    std::string newTexture = m_comboBoxRoughness->currentText().toStdString();

    AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
    m_material->setRoughnessTexture(instance.get(newTexture));
}

void WidgetMaterialPBR::onAOChanged()
{
    m_material->ao() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialPBR::onAOTextureChanged(int)
{
    std::string newTexture = m_comboBoxAO->currentText().toStdString();

    AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
    m_material->setAOTexture(instance.get(newTexture));
}

void WidgetMaterialPBR::onEmissiveChanged(double)
{
    m_material->emissive() = static_cast<float>(m_emissive->value());
}

void WidgetMaterialPBR::onNormalTextureChanged(int)
{
    std::string newTexture = m_comboBoxNormal->currentText().toStdString();

    AssetManager<std::string, Texture>& instance = AssetManager<std::string, Texture>::getInstance();
    m_material->setNormalTexture(instance.get(newTexture));
}

void WidgetMaterialPBR::onUTilingChanged(double) 
{
    m_material->uTiling() = m_uTiling->value();
}

void WidgetMaterialPBR::onVTilingChanged(double) 
{
    m_material->vTiling() = m_vTiling->value();
}


