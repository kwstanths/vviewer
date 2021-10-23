#include "WidgetMaterialPBR.hpp"

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include <iostream>

WidgetMaterialPBR::WidgetMaterialPBR(QWidget * parent, MaterialPBR * material) : QWidget(parent)
{
    m_material = material;

    m_colorButton = new QPushButton();
    m_colorButton->setFixedWidth(60);
    setColorButtonColor();
    QHBoxLayout * layout_albedo = new QHBoxLayout();
    layout_albedo->addWidget(new QLabel("Albedo:"));
    layout_albedo->addWidget(m_colorButton);
    layout_albedo->setContentsMargins(0, 0, 0, 0);
    QWidget * widget_albedo = new QWidget();
    widget_albedo->setLayout(layout_albedo);
    widget_albedo->setContentsMargins(0, 0, 0, 0);

    m_metallic = new QSlider(Qt::Horizontal);
    m_metallic->setMaximum(100);
    m_metallic->setMinimum(0);
    m_metallic->setSingleStep(1);
    m_metallic->setValue(material->getMetallic() * 100);
    m_metallic->setFixedWidth(150);
    QHBoxLayout * layout_metallic = new QHBoxLayout();
    layout_metallic->addWidget(new QLabel("Metallic:"));
    layout_metallic->setContentsMargins(0, 0, 0, 0);
    layout_metallic->addWidget(m_metallic);
    QWidget * widget_metallic = new QWidget();
    widget_metallic->setLayout(layout_metallic);
    widget_metallic->setContentsMargins(0, 0, 0, 0);

    m_roughness = new QSlider(Qt::Horizontal);
    m_roughness->setMaximum(100);
    m_roughness->setMinimum(0);
    m_roughness->setSingleStep(1);
    m_roughness->setValue(material->getRoughness() * 100);
    m_roughness->setFixedWidth(150);
    QHBoxLayout * layout_roughness = new QHBoxLayout();
    layout_roughness->addWidget(new QLabel("Roughness:"));
    layout_roughness->addWidget(m_roughness);
    layout_roughness->setContentsMargins(0, 0, 0, 0);
    QWidget * widget_roughness = new QWidget();
    widget_roughness->setLayout(layout_roughness);

    m_ao = new QSlider(Qt::Horizontal);
    m_ao->setMaximum(100);
    m_ao->setMinimum(0);
    m_ao->setSingleStep(1);
    m_ao->setValue(material->getAO() * 100);
    m_ao->setFixedWidth(150);
    QHBoxLayout * layout_ao = new QHBoxLayout();
    layout_ao->addWidget(new QLabel("Ambient:"));
    layout_ao->addWidget(m_ao);
    layout_ao->setContentsMargins(0, 0, 0, 0);
    QWidget * widget_ao = new QWidget();
    widget_ao->setLayout(layout_ao);

    m_emissive = new QSlider(Qt::Horizontal);
    m_emissive->setMaximum(100);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->getEmissive() * 100);
    m_emissive->setFixedWidth(150);
    QHBoxLayout * layout_emissive = new QHBoxLayout();
    layout_emissive->addWidget(new QLabel("Emissive:"));
    layout_emissive->addWidget(m_emissive);
    layout_emissive->setContentsMargins(0, 0, 0, 0);
    QWidget * widget_emissive = new QWidget();
    widget_emissive->setLayout(layout_emissive);

    QGroupBox * groupBox = new QGroupBox(tr("Material"));
    QVBoxLayout * layoutTest = new QVBoxLayout();
    layoutTest->addWidget(widget_albedo);
    layoutTest->addWidget(widget_metallic);
    layoutTest->addWidget(widget_roughness);
    layoutTest->addWidget(widget_ao);
    layoutTest->addWidget(widget_emissive);
    layoutTest->setContentsMargins(5, 5, 5, 5);
    layoutTest->setAlignment(Qt::AlignTop);

    groupBox->setLayout(layoutTest);

    QVBoxLayout * layoutMain = new QVBoxLayout();
    layoutMain->addWidget(groupBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);
    setLayout(layoutMain);
    setFixedHeight(145);

    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onColorButton()));
    connect(m_metallic, &QSlider::valueChanged, this, &WidgetMaterialPBR::onMetallicChanged);
    connect(m_roughness, &QSlider::valueChanged, this, &WidgetMaterialPBR::onRoughnessChanged);
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialPBR::onAOChanged);
    connect(m_emissive, &QSlider::valueChanged, this, &WidgetMaterialPBR::onEmissiveChanged);
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


