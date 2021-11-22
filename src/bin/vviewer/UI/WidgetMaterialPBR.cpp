#include "WidgetMaterialPBR.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include "core/AssetManager.hpp"

WidgetMaterialPBR::WidgetMaterialPBR(QWidget * parent, SceneObject * sceneObject, MaterialPBR * material, 
    QStringList availableMaterials, QStringList availableTextures) : QWidget(parent)
{
    m_sceneObject = sceneObject;
    m_material = material;
    m_comboBoxAvailableMaterials = new QComboBox();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->setCurrentText(QString::fromStdString(material->m_name));
    connect(m_comboBoxAvailableMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onMaterialChanged(int)));

    m_comboBoxAlbedo = new QComboBox();
    m_comboBoxAlbedo->addItems(availableTextures);
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
    m_comboBoxMetallic->addItems(availableTextures);
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
    m_comboBoxRoughness->addItems(availableTextures);
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
    m_comboBoxAO->addItems(availableTextures);
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


    m_comboBoxEmissive = new QComboBox();
    m_comboBoxEmissive->addItems(availableTextures);
    m_comboBoxEmissive->setCurrentText(QString::fromStdString(material->getEmissiveTexture()->m_name));
    connect(m_comboBoxEmissive, SIGNAL(currentIndexChanged(int)), this, SLOT(onEmissiveTextureChanged(int)));
    m_emissive = new QSlider(Qt::Horizontal);
    m_emissive->setMaximum(100);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->getEmissive() * 100);
    QGroupBox * groupBoxEmssive = new QGroupBox(tr("Emissive"));
    QVBoxLayout * layoutEmissive = new QVBoxLayout();
    layoutEmissive->addWidget(m_emissive);
    layoutEmissive->addWidget(m_comboBoxEmissive);
    layoutEmissive->setContentsMargins(5, 5, 5, 5);
    groupBoxEmssive->setLayout(layoutEmissive);


    m_comboBoxNormal = new QComboBox();
    m_comboBoxNormal->addItems(availableTextures);
    m_comboBoxNormal->setCurrentText(QString::fromStdString(material->getNormalTexture()->m_name));
    connect(m_comboBoxNormal, SIGNAL(currentIndexChanged(int)), this, SLOT(onNormalTextureChanged(int)));
    QGroupBox * groupBoxNormal = new QGroupBox(tr("Normal"));
    QVBoxLayout * layoutNormal = new QVBoxLayout();
    layoutNormal->addWidget(m_comboBoxNormal);
    layoutNormal->setContentsMargins(5, 5, 5, 5);
    groupBoxNormal->setLayout(layoutNormal);


    QGroupBox * groupBox = new QGroupBox(tr("Material"));
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(m_comboBoxAvailableMaterials);
    layoutTest->addWidget(groupBoxAlbedo);
    layoutTest->addWidget(groupBoxMetallic);
    layoutTest->addWidget(groupBoxRoughness);
    layoutTest->addWidget(groupBoxAO);
    layoutTest->addWidget(groupBoxEmssive);
    layoutTest->addWidget(groupBoxNormal);
    layoutTest->setContentsMargins(5, 5, 5, 5);
    layoutTest->setSpacing(15);
    layoutTest->setAlignment(Qt::AlignTop);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    setLayout(layoutMain);
    setFixedHeight(530);

    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onColorButton()));
    connect(m_metallic, &QSlider::valueChanged, this, &WidgetMaterialPBR::onMetallicChanged);
    connect(m_roughness, &QSlider::valueChanged, this, &WidgetMaterialPBR::onRoughnessChanged);
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialPBR::onAOChanged);
    connect(m_emissive, &QSlider::valueChanged, this, &WidgetMaterialPBR::onEmissiveChanged);
}

void WidgetMaterialPBR::onMaterialChanged(int)
{
    std::string newMaterial = m_comboBoxAvailableMaterials->currentText().toStdString();

    AssetManager<std::string, Material *>& instance = AssetManager<std::string, Material *>::getInstance();
    Material * material = instance.Get(newMaterial);

    m_sceneObject->setMaterial(material);
    m_material = static_cast<MaterialPBR *>(material);
    /* TODO what if it's not this kind of material? */

    setColorButtonColor();
    m_comboBoxAlbedo->setCurrentText(QString::fromStdString(m_material->getAlbedoTexture()->m_name));
    m_metallic->setValue(m_material->getMetallic() * 100);
    m_comboBoxMetallic->setCurrentText(QString::fromStdString(m_material->getMetallicTexture()->m_name));
    m_roughness->setValue(m_material->getRoughness() * 100);
    m_comboBoxRoughness->setCurrentText(QString::fromStdString(m_material->getRoughnessTexture()->m_name));
    m_ao->setValue(m_material->getAO() * 100);
    m_comboBoxAO->setCurrentText(QString::fromStdString(m_material->getAOTexture()->m_name));
    m_emissive->setValue(m_material->getEmissive() * 100);
    m_comboBoxEmissive->setCurrentText(QString::fromStdString(m_material->getEmissiveTexture()->m_name));
}

void WidgetMaterialPBR::onColorButton()
{
    QColorDialog * dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onColorChanged(QColor)));
    dialog->exec();

    setColorButtonColor();
}

void WidgetMaterialPBR::setColorButtonColor()
{
    glm::vec4 currentColor = m_material->getAlbedo();
    QPalette pal = m_colorButton->palette();
    pal.setColor(QPalette::Button, QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255));
    m_colorButton->setAutoFillBackground(true);
    m_colorButton->setPalette(pal);
    m_colorButton->update();
}

void WidgetMaterialPBR::onColorChanged(QColor color) {
    m_material->getAlbedo() = glm::vec4(color.red(), color.green(), color.blue(), color.alpha()) / glm::vec4(255.0f);
}

void WidgetMaterialPBR::onColorTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlbedo->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setAlbedoTexture(texture);
}

void WidgetMaterialPBR::onMetallicChanged()
{
    m_material->getMetallic() = static_cast<float>(m_metallic->value()) / 100;
}

void WidgetMaterialPBR::onMetallicTextureChanged(int)
{
    std::string newTexture = m_comboBoxMetallic->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setMetallicTexture(texture);
}

void WidgetMaterialPBR::onRoughnessChanged()
{
    m_material->getRoughness() = static_cast<float>(m_roughness->value()) / 100;
}

void WidgetMaterialPBR::onRoughnessTextureChanged(int)
{
    std::string newTexture = m_comboBoxRoughness->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setRoughnessTexture(texture);
}

void WidgetMaterialPBR::onAOChanged()
{
    m_material->getAO() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialPBR::onAOTextureChanged(int)
{
    std::string newTexture = m_comboBoxAO->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setAOTexture(texture);
}

void WidgetMaterialPBR::onEmissiveChanged()
{
    m_material->getEmissive() = static_cast<float>(m_emissive->value()) / 100;
}

void WidgetMaterialPBR::onEmissiveTextureChanged(int)
{
    std::string newTexture = m_comboBoxEmissive->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setEmissiveTexture(texture);
}

void WidgetMaterialPBR::onNormalTextureChanged(int)
{
    std::string newTexture = m_comboBoxNormal->currentText().toStdString();

    AssetManager<std::string, Texture *>& instance = AssetManager<std::string, Texture *>::getInstance();
    Texture * texture = instance.Get(newTexture);

    m_material->setNormalTexture(texture);
}


