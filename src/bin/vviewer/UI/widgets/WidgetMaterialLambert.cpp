#include "WidgetMaterialLambert.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include "vengine/core/AssetManager.hpp"
#include "vengine/core/Materials.hpp"

#include "UI/UIUtils.hpp"

using namespace vengine;

WidgetMaterialLambert::WidgetMaterialLambert(QWidget *parent, MaterialLambert *material)
    : QWidget(parent)
{
    m_material = material;

    QStringList availableColorTextures = getImportedTextures(ColorSpace::sRGB);
    QStringList availableLinearTextures = getImportedTextures(ColorSpace::LINEAR);

    m_comboBoxAlbedo = new QComboBox();
    m_comboBoxAlbedo->addItems(availableColorTextures);
    m_comboBoxAlbedo->setCurrentText(QString::fromStdString(material->getAlbedoTexture()->name()));
    connect(m_comboBoxAlbedo, SIGNAL(currentIndexChanged(int)), this, SLOT(onAlbedoTextureChanged(int)));
    m_colorAlbedo = new QPushButton();
    m_colorAlbedo->setFixedWidth(25);
    setColorButton(m_colorAlbedo, material->albedo());
    QGroupBox *groupBoxAlbedo = new QGroupBox(tr("Albedo"));
    QHBoxLayout *layoutAlbedo = new QHBoxLayout();
    layoutAlbedo->addWidget(m_colorAlbedo);
    layoutAlbedo->addWidget(m_comboBoxAlbedo);
    layoutAlbedo->setContentsMargins(5, 5, 5, 5);
    groupBoxAlbedo->setLayout(layoutAlbedo);

    m_comboBoxAO = new QComboBox();
    m_comboBoxAO->addItems(availableLinearTextures);
    m_comboBoxAO->setCurrentText(QString::fromStdString(material->getAOTexture()->name()));
    connect(m_comboBoxAO, SIGNAL(currentIndexChanged(int)), this, SLOT(onAOTextureChanged(int)));
    m_ao = new QSlider(Qt::Horizontal);
    m_ao->setMaximum(100);
    m_ao->setMinimum(0);
    m_ao->setSingleStep(1);
    m_ao->setValue(material->ao() * 100);
    QGroupBox *groupBoxAO = new QGroupBox(tr("Ambient"));
    QVBoxLayout *layoutAO = new QVBoxLayout();
    layoutAO->addWidget(m_ao);
    layoutAO->addWidget(m_comboBoxAO);
    layoutAO->setContentsMargins(5, 5, 5, 5);
    groupBoxAO->setLayout(layoutAO);

    m_comboBoxEmissive = new QComboBox();
    m_comboBoxEmissive->addItems(availableColorTextures);
    m_comboBoxEmissive->setCurrentText(QString::fromStdString(material->getEmissiveTexture()->name()));
    connect(m_comboBoxEmissive, SIGNAL(currentIndexChanged(int)), this, SLOT(onEmissiveTextureChanged(int)));
    m_colorEmissive = new QPushButton();
    m_colorEmissive->setFixedWidth(25);
    setColorButton(m_colorEmissive, material->emissive());
    QHBoxLayout *layoutEmissiveColor = new QHBoxLayout();
    layoutEmissiveColor->addWidget(m_colorEmissive);
    layoutEmissiveColor->addWidget(m_comboBoxEmissive);
    layoutEmissiveColor->setContentsMargins(5, 5, 5, 5);
    QWidget *widgetEmissveColor = new QWidget();
    widgetEmissveColor->setLayout(layoutEmissiveColor);

    m_emissive = new QDoubleSpinBox();
    m_emissive->setMaximum(10000);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->emissiveIntensity());
    QGroupBox *groupBoxEmssive = new QGroupBox(tr("Emissive"));
    QVBoxLayout *layoutEmissive = new QVBoxLayout();
    layoutEmissive->addWidget(widgetEmissveColor);
    layoutEmissive->addWidget(m_emissive);
    layoutEmissive->setContentsMargins(5, 5, 5, 5);
    groupBoxEmssive->setLayout(layoutEmissive);

    m_comboBoxNormal = new QComboBox();
    m_comboBoxNormal->addItems(availableLinearTextures);
    m_comboBoxNormal->setCurrentText(QString::fromStdString(material->getNormalTexture()->name()));
    connect(m_comboBoxNormal, SIGNAL(currentIndexChanged(int)), this, SLOT(onNormalTextureChanged(int)));
    QGroupBox *groupBoxNormal = new QGroupBox(tr("Normal"));
    QVBoxLayout *layoutNormal = new QVBoxLayout();
    layoutNormal->addWidget(m_comboBoxNormal);
    layoutNormal->setContentsMargins(5, 5, 5, 5);
    groupBoxNormal->setLayout(layoutNormal);

    m_uTiling = new QDoubleSpinBox();
    m_uTiling->setMinimum(0);
    m_uTiling->setMaximum(65536);
    m_uTiling->setValue(material->uTiling());
    connect(m_uTiling, SIGNAL(valueChanged(double)), this, SLOT(onUTilingChanged(double)));
    m_vTiling = new QDoubleSpinBox();
    m_vTiling->setMinimum(0);
    m_vTiling->setMaximum(65536);
    m_vTiling->setValue(material->vTiling());
    connect(m_vTiling, SIGNAL(valueChanged(double)), this, SLOT(onVTilingChanged(double)));
    QGroupBox *groupBoxUVTiling = new QGroupBox(tr("UV tiling"));
    QHBoxLayout *layoutUVTiling = new QHBoxLayout();
    layoutUVTiling->addWidget(new QLabel("U:"));
    layoutUVTiling->addWidget(m_uTiling);
    layoutUVTiling->addWidget(new QLabel("V: "));
    layoutUVTiling->addWidget(m_vTiling);
    layoutUVTiling->setContentsMargins(5, 5, 5, 5);
    groupBoxUVTiling->setLayout(layoutUVTiling);

    auto itr = materialTypeNames.find(MaterialType::MATERIAL_LAMBERT);
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(QString::fromStdString("Type: " + itr->second)));
    layoutMain->addWidget(groupBoxAlbedo);
    layoutMain->addWidget(groupBoxAO);
    layoutMain->addWidget(groupBoxEmssive);
    layoutMain->addWidget(groupBoxNormal);
    layoutMain->addWidget(groupBoxUVTiling);
    layoutMain->setContentsMargins(5, 5, 5, 5);
    layoutMain->setSpacing(15);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
    setFixedHeight(HEIGHT);

    connect(m_colorAlbedo, SIGNAL(pressed()), this, SLOT(onAlbedoButton()));
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialLambert::onAOChanged);
    connect(m_emissive, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialLambert::onEmissiveChanged);
    connect(m_colorEmissive, &QPushButton::pressed, this, &WidgetMaterialLambert::onEmissiveButton);
}

void WidgetMaterialLambert::onAlbedoButton()
{
    glm::vec4 currentColor = m_material->albedo();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog *dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);
    dialog->exec();

    QColor color = dialog->currentColor();
    m_material->albedo() = glm::vec4(color.red(), color.green(), color.blue(), color.alpha()) / glm::vec4(255.0);

    setColorButton(m_colorAlbedo, m_material->albedo());
}

void WidgetMaterialLambert::setColorButton(QPushButton *button, const glm::vec4 &color)
{
    QColor col = QColor(color.r * 255, color.g * 255, color.b * 255);
    QString qss = QString("background-color: %1").arg(col.name());
    button->setStyleSheet(qss);

    /*QPalette pal = m_colorButton->palette();
    pal.setColor(QPalette::Button, QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255));
    m_colorButton->setAutoFillBackground(true);
    m_colorButton->setPalette(pal);
    m_colorButton->update();*/
}

void WidgetMaterialLambert::onAlbedoTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlbedo->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setAlbedoTexture(textures.get(newTexture));
}

void WidgetMaterialLambert::onAOChanged()
{
    m_material->ao() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialLambert::onAOTextureChanged(int)
{
    std::string newTexture = m_comboBoxAO->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setAOTexture(textures.get(newTexture));
}

void WidgetMaterialLambert::onEmissiveChanged(double)
{
    m_material->emissiveIntensity() = static_cast<float>(m_emissive->value());
}

void WidgetMaterialLambert::onEmissiveButton()
{
    glm::vec4 currentColor = m_material->emissive();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog *dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);
    dialog->exec();

    QColor color = dialog->currentColor();
    m_material->emissive() = glm::vec4(color.red(), color.green(), color.blue(), 255.0F) / glm::vec4(255.0f);

    setColorButton(m_colorEmissive, m_material->emissive());

    delete dialog;
}

void WidgetMaterialLambert::onEmissiveTextureChanged(int)
{
    std::string newTexture = m_comboBoxEmissive->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setEmissiveTexture(textures.get(newTexture));
}

void WidgetMaterialLambert::onNormalTextureChanged(int)
{
    std::string newTexture = m_comboBoxNormal->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();

    m_material->setNormalTexture(textures.get(newTexture));
}

void WidgetMaterialLambert::onUTilingChanged(double)
{
    m_material->uTiling() = m_uTiling->value();
}

void WidgetMaterialLambert::onVTilingChanged(double)
{
    m_material->vTiling() = m_vTiling->value();
}