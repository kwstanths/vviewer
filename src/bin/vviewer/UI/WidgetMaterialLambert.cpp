#include "WidgetMaterialLambert.hpp"

#include <iostream>

#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcolordialog.h>

#include <glm/glm.hpp>

#include "core/AssetManager.hpp"
#include "core/Materials.hpp"

#include "UIUtils.hpp"

WidgetMaterialLambert::WidgetMaterialLambert(QWidget* parent, MaterialLambert* material) : QWidget(parent)
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
    QGroupBox* groupBoxAlbedo = new QGroupBox(tr("Albedo"));
    QHBoxLayout* layoutAlbedo = new QHBoxLayout();
    layoutAlbedo->addWidget(m_colorButton);
    layoutAlbedo->addWidget(m_comboBoxAlbedo);
    layoutAlbedo->setContentsMargins(5, 5, 5, 5);
    groupBoxAlbedo->setLayout(layoutAlbedo);


    m_comboBoxAO = new QComboBox();
    m_comboBoxAO->addItems(availableLinearTextures);
    m_comboBoxAO->setCurrentText(QString::fromStdString(material->getAOTexture()->m_name));
    connect(m_comboBoxAO, SIGNAL(currentIndexChanged(int)), this, SLOT(onAOTextureChanged(int)));
    m_ao = new QSlider(Qt::Horizontal);
    m_ao->setMaximum(100);
    m_ao->setMinimum(0);
    m_ao->setSingleStep(1);
    m_ao->setValue(material->getAO() * 100);
    QGroupBox* groupBoxAO = new QGroupBox(tr("Ambient"));
    QVBoxLayout* layoutAO = new QVBoxLayout();
    layoutAO->addWidget(m_ao);
    layoutAO->addWidget(m_comboBoxAO);
    layoutAO->setContentsMargins(5, 5, 5, 5);
    groupBoxAO->setLayout(layoutAO);


    m_comboBoxEmissive = new QComboBox();
    m_comboBoxEmissive->addItems(availableLinearTextures);
    m_comboBoxEmissive->setCurrentText(QString::fromStdString(material->getEmissiveTexture()->m_name));
    connect(m_comboBoxEmissive, SIGNAL(currentIndexChanged(int)), this, SLOT(onEmissiveTextureChanged(int)));
    m_emissive = new QSlider(Qt::Horizontal);
    m_emissive->setMaximum(100);
    m_emissive->setMinimum(0);
    m_emissive->setSingleStep(1);
    m_emissive->setValue(material->getEmissive() * 100);
    QGroupBox* groupBoxEmssive = new QGroupBox(tr("Emissive"));
    QVBoxLayout* layoutEmissive = new QVBoxLayout();
    layoutEmissive->addWidget(m_emissive);
    layoutEmissive->addWidget(m_comboBoxEmissive);
    layoutEmissive->setContentsMargins(5, 5, 5, 5);
    groupBoxEmssive->setLayout(layoutEmissive);


    m_comboBoxNormal = new QComboBox();
    m_comboBoxNormal->addItems(availableLinearTextures);
    m_comboBoxNormal->setCurrentText(QString::fromStdString(material->getNormalTexture()->m_name));
    connect(m_comboBoxNormal, SIGNAL(currentIndexChanged(int)), this, SLOT(onNormalTextureChanged(int)));
    QGroupBox* groupBoxNormal = new QGroupBox(tr("Normal"));
    QVBoxLayout* layoutNormal = new QVBoxLayout();
    layoutNormal->addWidget(m_comboBoxNormal);
    layoutNormal->setContentsMargins(5, 5, 5, 5);
    groupBoxNormal->setLayout(layoutNormal);

    auto itr = materialTypeNames.find(MaterialType::MATERIAL_LAMBERT);
    QVBoxLayout* layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(QString::fromStdString("Type: " + itr->second)));
    layoutMain->addWidget(groupBoxAlbedo);
    layoutMain->addWidget(groupBoxAO);
    layoutMain->addWidget(groupBoxEmssive);
    layoutMain->addWidget(groupBoxNormal);
    layoutMain->setContentsMargins(5, 5, 5, 5);
    layoutMain->setSpacing(15);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
    setFixedHeight(480);

    connect(m_colorButton, SIGNAL(pressed()), this, SLOT(onColorButton()));
    connect(m_ao, &QSlider::valueChanged, this, &WidgetMaterialLambert::onAOChanged);
    connect(m_emissive, &QSlider::valueChanged, this, &WidgetMaterialLambert::onEmissiveChanged);
}

void WidgetMaterialLambert::onColorButton()
{
    glm::vec4 currentColor = m_material->getAlbedo();
    QColor col = QColor(currentColor.r * 255, currentColor.g * 255, currentColor.b * 255);

    QColorDialog* dialog = new QColorDialog(nullptr);
    dialog->adjustSize();
    dialog->setCurrentColor(col);

    connect(dialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(onColorChanged(QColor)));
    dialog->exec();

    /* When the window exits, change the color of the UI button */
    setColorButtonColor();
}

void WidgetMaterialLambert::setColorButtonColor()
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

void WidgetMaterialLambert::onColorChanged(QColor color) {
    m_material->albedo() = glm::vec4(color.red(), color.green(), color.blue(), color.alpha()) / glm::vec4(255.0f);
}

void WidgetMaterialLambert::onColorTextureChanged(int)
{
    std::string newTexture = m_comboBoxAlbedo->currentText().toStdString();

    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    Texture* texture = instance.Get(newTexture);

    m_material->setAlbedoTexture(texture);
}

void WidgetMaterialLambert::onAOChanged()
{
    m_material->ao() = static_cast<float>(m_ao->value()) / 100;
}

void WidgetMaterialLambert::onAOTextureChanged(int)
{
    std::string newTexture = m_comboBoxAO->currentText().toStdString();

    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    Texture* texture = instance.Get(newTexture);

    m_material->setAOTexture(texture);
}

void WidgetMaterialLambert::onEmissiveChanged()
{
    m_material->emissive() = static_cast<float>(m_emissive->value()) / 100;
}

void WidgetMaterialLambert::onEmissiveTextureChanged(int)
{
    std::string newTexture = m_comboBoxEmissive->currentText().toStdString();

    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    Texture* texture = instance.Get(newTexture);

    m_material->setEmissiveTexture(texture);
}

void WidgetMaterialLambert::onNormalTextureChanged(int)
{
    std::string newTexture = m_comboBoxNormal->currentText().toStdString();

    AssetManager<std::string, Texture*>& instance = AssetManager<std::string, Texture*>::getInstance();
    Texture* texture = instance.Get(newTexture);

    m_material->setNormalTexture(texture);
}


