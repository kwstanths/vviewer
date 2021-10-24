#include "WidgetMaterialPBR.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include "core/AssetManager.hpp"

WidgetMaterialPBR::WidgetMaterialPBR(QWidget * parent, SceneObject * sceneObject, MaterialPBR * material, QStringList availableMaterials) : QWidget(parent)
{
    m_sceneObject = sceneObject;
    m_material = material;

    m_comboBoxAvailableMaterials = new QComboBox();
    m_comboBoxAvailableMaterials->addItems(availableMaterials);
    m_comboBoxAvailableMaterials->setCurrentText(QString::fromStdString(material->m_name));
    connect(m_comboBoxAvailableMaterials, SIGNAL(currentIndexChanged(int)), this, SLOT(onMaterialChanged(int)));

    QGroupBox * groupBoxMaterials = new QGroupBox(tr("Material"));
    QVBoxLayout * layoutMaterials = new QVBoxLayout();
    layoutMaterials->addWidget(m_comboBoxAvailableMaterials);
    layoutMaterials->setContentsMargins(5, 5, 5, 5);
    groupBoxMaterials->setLayout(layoutMaterials);

    m_colorButton = new QPushButton();
    m_colorButton->setFixedWidth(60);
    setColorButtonColor();
    QHBoxLayout * layoutAlbedo = new QHBoxLayout();
    layoutAlbedo->addWidget(new QLabel("Albedo:"));
    layoutAlbedo->addWidget(m_colorButton);
    layoutAlbedo->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetAlbedo = new QWidget();
    widgetAlbedo->setLayout(layoutAlbedo);
    widgetAlbedo->setContentsMargins(0, 0, 0, 0);

    m_metallic = new QSlider(Qt::Horizontal);
    m_metallic->setMaximum(100);
    m_metallic->setMinimum(0);
    m_metallic->setSingleStep(1);
    m_metallic->setValue(material->getMetallic() * 100);
    m_metallic->setFixedWidth(150);
    QHBoxLayout * layoutMetallic = new QHBoxLayout();
    layoutMetallic->addWidget(new QLabel("Metallic:"));
    layoutMetallic->setContentsMargins(0, 0, 0, 0);
    layoutMetallic->addWidget(m_metallic);
    QWidget * widgetMetallic = new QWidget();
    widgetMetallic->setLayout(layoutMetallic);
    widgetMetallic->setContentsMargins(0, 0, 0, 0);

    m_roughness = new QSlider(Qt::Horizontal);
    m_roughness->setMaximum(100);
    m_roughness->setMinimum(0);
    m_roughness->setSingleStep(1);
    m_roughness->setValue(material->getRoughness() * 100);
    m_roughness->setFixedWidth(150);
    QHBoxLayout * layoutRoughness = new QHBoxLayout();
    layoutRoughness->addWidget(new QLabel("Roughness:"));
    layoutRoughness->addWidget(m_roughness);
    layoutRoughness->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetRoughness = new QWidget();
    widgetRoughness->setLayout(layoutRoughness);

    m_ao = new QSlider(Qt::Horizontal);
    m_ao->setMaximum(100);
    m_ao->setMinimum(0);
    m_ao->setSingleStep(1);
    m_ao->setValue(material->getAO() * 100);
    m_ao->setFixedWidth(150);
    QHBoxLayout * layoutAo = new QHBoxLayout();
    layoutAo->addWidget(new QLabel("Ambient:"));
    layoutAo->addWidget(m_ao);
    layoutAo->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetAo = new QWidget();
    widgetAo->setLayout(layoutAo);

    m_emissive = new QSlider(Qt::Horizontal);
    m_emissive->setMaximum(100);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->getEmissive() * 100);
    m_emissive->setFixedWidth(150);
    QHBoxLayout * layoutEmissive = new QHBoxLayout();
    layoutEmissive->addWidget(new QLabel("Emissive:"));
    layoutEmissive->addWidget(m_emissive);
    layoutEmissive->setContentsMargins(0, 0, 0, 0);
    QWidget * widgetEmissive = new QWidget();
    widgetEmissive->setLayout(layoutEmissive);

    QGroupBox * groupBox = new QGroupBox(tr("Material"));
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(groupBoxMaterials);
    layoutTest->addWidget(widgetAlbedo);
    layoutTest->addWidget(widgetMetallic);
    layoutTest->addWidget(widgetRoughness);
    layoutTest->addWidget(widgetAo);
    layoutTest->addWidget(widgetEmissive);
    layoutTest->setContentsMargins(5, 5, 5, 5);
    layoutTest->setAlignment(Qt::AlignTop);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    setLayout(layoutMain);
    setFixedHeight(200);

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
    m_metallic->setValue(m_material->getMetallic() * 100);
    m_roughness->setValue(m_material->getRoughness() * 100);
    m_ao->setValue(m_material->getAO() * 100);
    m_emissive->setValue(m_material->getEmissive() * 100);
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

void WidgetMaterialPBR::onMetallicChanged()
{
    m_material->getMetallic() = static_cast<float>(m_metallic->value()) / 100;
}

void WidgetMaterialPBR::onRoughnessChanged()
{
    m_material->getRoughness() = static_cast<float>(m_roughness->value()) / 100;
}

void WidgetMaterialPBR::onAOChanged()
{
    m_material->getAO() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialPBR::onEmissiveChanged()
{
    m_material->getEmissive() = static_cast<float>(m_emissive->value()) / 100;
}


