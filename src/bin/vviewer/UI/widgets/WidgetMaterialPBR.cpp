#include "WidgetMaterialPBR.hpp"

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

WidgetMaterialPBR::WidgetMaterialPBR(QWidget *parent, MaterialPBRStandard *material)
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
    setButtonColor(m_colorAlbedo, material->albedo());
    QGroupBox *groupBoxAlbedo = new QGroupBox(tr("Albedo"));
    QHBoxLayout *layoutAlbedo = new QHBoxLayout();
    layoutAlbedo->addWidget(m_colorAlbedo);
    layoutAlbedo->addWidget(m_comboBoxAlbedo);
    layoutAlbedo->setContentsMargins(5, 5, 5, 5);
    groupBoxAlbedo->setLayout(layoutAlbedo);

    m_comboBoxMetallic = new QComboBox();
    m_comboBoxMetallic->addItems(availableLinearTextures);
    m_comboBoxMetallic->setCurrentText(QString::fromStdString(material->getMetallicTexture()->name()));
    connect(m_comboBoxMetallic, SIGNAL(currentIndexChanged(int)), this, SLOT(onMetallicTextureChanged(int)));
    m_metallic = new QSlider(Qt::Horizontal);
    m_metallic->setMaximum(100);
    m_metallic->setMinimum(0);
    m_metallic->setSingleStep(1);
    m_metallic->setValue(material->metallic() * 100);
    QGroupBox *groupBoxMetallic = new QGroupBox(tr("Metallic"));
    QVBoxLayout *layoutMetallic = new QVBoxLayout();
    layoutMetallic->addWidget(m_metallic);
    layoutMetallic->addWidget(m_comboBoxMetallic);
    layoutMetallic->setContentsMargins(5, 5, 5, 5);
    groupBoxMetallic->setLayout(layoutMetallic);

    m_comboBoxRoughness = new QComboBox();
    m_comboBoxRoughness->addItems(availableLinearTextures);
    m_comboBoxRoughness->setCurrentText(QString::fromStdString(material->getRoughnessTexture()->name()));
    connect(m_comboBoxRoughness, SIGNAL(currentIndexChanged(int)), this, SLOT(onRoughnessTextureChanged(int)));
    m_roughness = new QSlider(Qt::Horizontal);
    m_roughness->setMaximum(100);
    m_roughness->setMinimum(0);
    m_roughness->setSingleStep(1);
    m_roughness->setValue(material->roughness() * 100);
    QGroupBox *groupBoxRoughness = new QGroupBox(tr("Roughness"));
    QVBoxLayout *layoutRoughness = new QVBoxLayout();
    layoutRoughness->addWidget(m_roughness);
    layoutRoughness->addWidget(m_comboBoxRoughness);
    layoutRoughness->setContentsMargins(5, 5, 5, 5);
    groupBoxRoughness->setLayout(layoutRoughness);

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
    setButtonColor(m_colorEmissive, material->emissive());
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

    m_comboBoxAlpha = new QComboBox();
    m_comboBoxAlpha->addItems(availableLinearTextures);
    m_comboBoxAlpha->setCurrentText(QString::fromStdString(material->getAlphaTexture()->name()));
    connect(m_comboBoxAlpha, SIGNAL(currentIndexChanged(int)), this, SLOT(onAlphaTextureChanged(int)));
    m_alpha = new QSlider(Qt::Horizontal);
    m_alpha->setMaximum(100);
    m_alpha->setMinimum(0);
    m_alpha->setSingleStep(1);
    m_alpha->setValue(material->albedo().a * 100);
    m_groupBoxAlpha = new QGroupBox(tr("Alpha"));
    m_groupBoxAlpha->setCheckable(true);
    m_groupBoxAlpha->setChecked(material->isTransparent());
    connect(m_groupBoxAlpha, SIGNAL(clicked(bool)), this, SLOT(onAlphaStateChanged(bool)));
    QVBoxLayout *layoutAlpha = new QVBoxLayout();
    layoutAlpha->addWidget(m_alpha);
    layoutAlpha->addWidget(m_comboBoxAlpha);
    layoutAlpha->setContentsMargins(5, 5, 5, 5);
    m_groupBoxAlpha->setLayout(layoutAlpha);

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

    auto itr = materialTypeNames.find(MaterialType::MATERIAL_PBR_STANDARD);
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(QString::fromStdString("Type: " + itr->second)));
    layoutMain->addWidget(groupBoxAlbedo);
    layoutMain->addWidget(groupBoxMetallic);
    layoutMain->addWidget(groupBoxRoughness);
    layoutMain->addWidget(groupBoxAO);
    layoutMain->addWidget(groupBoxEmssive);
    layoutMain->addWidget(groupBoxNormal);
    layoutMain->addWidget(m_groupBoxAlpha);
    layoutMain->addWidget(groupBoxUVTiling);
    layoutMain->setSpacing(15);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
    setFixedHeight(HEIGHT);

    connect(m_colorAlbedo, &QPushButton::pressed, this, &WidgetMaterialPBR::onAlbedoButton);
    connect(m_metallic, &QSlider::valueChanged, this, &WidgetMaterialPBR::onMetallicChanged);
    connect(m_roughness, &QSlider::valueChanged, this, &WidgetMaterialPBR::onRoughnessChanged);
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialPBR::onAOChanged);
    connect(m_emissive, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialPBR::onEmissiveChanged);
    connect(m_colorEmissive, &QPushButton::pressed, this, &WidgetMaterialPBR::onEmissiveButton);
    connect(m_alpha, &QSlider::valueChanged, this, &WidgetMaterialPBR::onAlphaChanged);
}

void WidgetMaterialPBR::updateAvailableTextures()
{
    QStringList availableSRGBTextures = getImportedTextures(ColorSpace::sRGB);
    QStringList availableLinearTextures = getImportedTextures(ColorSpace::LINEAR);

    updateTextureComboBox(m_comboBoxAlbedo, availableSRGBTextures, QString::fromStdString(m_material->getAlbedoTexture()->name()));
    updateTextureComboBox(
        m_comboBoxMetallic, availableLinearTextures, QString::fromStdString(m_material->getMetallicTexture()->name()));
    updateTextureComboBox(
        m_comboBoxRoughness, availableLinearTextures, QString::fromStdString(m_material->getRoughnessTexture()->name()));
    updateTextureComboBox(m_comboBoxAO, availableLinearTextures, QString::fromStdString(m_material->getAOTexture()->name()));
    updateTextureComboBox(m_comboBoxEmissive, availableSRGBTextures, QString::fromStdString(m_material->getEmissiveTexture()->name()));
    updateTextureComboBox(m_comboBoxNormal, availableLinearTextures, QString::fromStdString(m_material->getNormalTexture()->name()));
    updateTextureComboBox(m_comboBoxAlpha, availableLinearTextures, QString::fromStdString(m_material->getAlphaTexture()->name()));
}

void WidgetMaterialPBR::onAlbedoButton()
{
    glm::vec4 currentColor = m_material->albedo();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog *dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);
    dialog->exec();

    QColor color = dialog->currentColor();
    float oldAlpha = m_material->albedo().a;
    m_material->albedo() = glm::vec4(glm::vec3(color.red(), color.green(), color.blue()) / glm::vec3(255.0f), oldAlpha);

    setButtonColor(m_colorAlbedo, m_material->albedo());

    delete dialog;
}

void WidgetMaterialPBR::onAlbedoTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlbedo->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setAlbedoTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onMetallicChanged()
{
    m_material->metallic() = static_cast<float>(m_metallic->value()) / 100;
}

void WidgetMaterialPBR::onMetallicTextureChanged(int)
{
    std::string newTexture = m_comboBoxMetallic->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setMetallicTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onRoughnessChanged()
{
    m_material->roughness() = static_cast<float>(m_roughness->value()) / 100;
}

void WidgetMaterialPBR::onRoughnessTextureChanged(int)
{
    std::string newTexture = m_comboBoxRoughness->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setRoughnessTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onAOChanged()
{
    m_material->ao() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialPBR::onAOTextureChanged(int)
{
    std::string newTexture = m_comboBoxAO->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setAOTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onEmissiveChanged(double)
{
    m_material->emissiveIntensity() = static_cast<float>(m_emissive->value());
}

void WidgetMaterialPBR::onEmissiveButton()
{
    glm::vec4 currentColor = m_material->emissive();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog *dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);
    dialog->exec();

    QColor color = dialog->currentColor();
    m_material->emissive() = glm::vec4(color.red(), color.green(), color.blue(), 255.0F) / glm::vec4(255.0f);

    setButtonColor(m_colorEmissive, m_material->emissive());

    delete dialog;
}

void WidgetMaterialPBR::onEmissiveTextureChanged(int)
{
    std::string newTexture = m_comboBoxEmissive->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setEmissiveTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onNormalTextureChanged(int)
{
    std::string newTexture = m_comboBoxNormal->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setNormalTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onAlphaStateChanged(bool)
{
    m_material->setTransparent(m_groupBoxAlpha->isChecked());
}

void WidgetMaterialPBR::onAlphaChanged()
{
    m_material->albedo().a = static_cast<float>(m_alpha->value()) / 100;
}

void WidgetMaterialPBR::onAlphaTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlpha->currentText().toStdString();

    auto &textures = AssetManager::getInstance().texturesMap();
    m_material->setAlphaTexture(textures.get(newTexture));
}

void WidgetMaterialPBR::onUTilingChanged(double)
{
    m_material->uTiling() = m_uTiling->value();
}

void WidgetMaterialPBR::onVTilingChanged(double)
{
    m_material->vTiling() = m_vTiling->value();
}
