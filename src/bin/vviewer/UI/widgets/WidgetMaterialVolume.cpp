#include "WidgetMaterialVolume.hpp"

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

WidgetMaterialVolume::WidgetMaterialVolume(QWidget *parent, MaterialVolume *material)
    : QWidget(parent)
{
    m_material = material;

    m_sigmaS_R = createSpinBox(material->sigmaS().r);
    m_sigmaS_G = createSpinBox(material->sigmaS().g);
    m_sigmaS_B = createSpinBox(material->sigmaS().b);
    m_sigmaS_RGB = createSpinBox(glm::max(glm::max(material->sigmaS().b, material->sigmaS().g), material->sigmaS().r));
    m_sigmaA_R = createSpinBox(material->sigmaA().r);
    m_sigmaA_G = createSpinBox(material->sigmaA().g);
    m_sigmaA_B = createSpinBox(material->sigmaA().b);
    m_sigmaA_RGB = createSpinBox(glm::max(glm::max(material->sigmaA().b, material->sigmaA().g), material->sigmaA().r));

    QHBoxLayout *layoutSRGB = new QHBoxLayout();
    layoutSRGB->addWidget(new QLabel("R: "));
    layoutSRGB->addWidget(m_sigmaS_R);
    layoutSRGB->addWidget(new QLabel("G: "));
    layoutSRGB->addWidget(m_sigmaS_G);
    layoutSRGB->addWidget(new QLabel("B: "));
    layoutSRGB->addWidget(m_sigmaS_B);
    layoutSRGB->setContentsMargins(0, 0, 0, 0);
    m_widgetSigmaSRGB = new QWidget();
    m_widgetSigmaSRGB->setLayout(layoutSRGB);
    QHBoxLayout *layoutSGrayscale = new QHBoxLayout();
    layoutSGrayscale->addWidget(new QLabel("S: "));
    layoutSGrayscale->addWidget(m_sigmaS_RGB);
    layoutSGrayscale->setContentsMargins(0, 0, 0, 0);
    m_widgetSigmaSGrayscale = new QWidget();
    m_widgetSigmaSGrayscale->setLayout(layoutSGrayscale);
    m_widgetSigmaSGrayscale->setFixedWidth(110);

    QHBoxLayout *layoutS = new QHBoxLayout();
    layoutS->addWidget(m_widgetSigmaSRGB);
    layoutS->addWidget(m_widgetSigmaSGrayscale);
    layoutS->setAlignment(Qt::AlignLeft);
    QGroupBox *groupBoxS = new QGroupBox(tr("Sigma scattering"));
    groupBoxS->setLayout(layoutS);

    QHBoxLayout *layoutARGB = new QHBoxLayout();
    layoutARGB->addWidget(new QLabel("R: "));
    layoutARGB->addWidget(m_sigmaA_R);
    layoutARGB->addWidget(new QLabel("G: "));
    layoutARGB->addWidget(m_sigmaA_G);
    layoutARGB->addWidget(new QLabel("B: "));
    layoutARGB->addWidget(m_sigmaA_B);
    layoutARGB->setContentsMargins(0, 0, 0, 0);
    m_widgetSigmaARGB = new QWidget();
    m_widgetSigmaARGB->setLayout(layoutARGB);
    QHBoxLayout *layoutAGrayscale = new QHBoxLayout();
    layoutAGrayscale->addWidget(new QLabel("A: "));
    layoutAGrayscale->addWidget(m_sigmaA_RGB);
    layoutAGrayscale->setContentsMargins(0, 0, 0, 0);
    m_widgetSigmaAGrayscale = new QWidget();
    m_widgetSigmaAGrayscale->setLayout(layoutAGrayscale);
    m_widgetSigmaAGrayscale->setFixedWidth(110);

    QHBoxLayout *layoutA = new QHBoxLayout();
    layoutA->addWidget(m_widgetSigmaARGB);
    layoutA->addWidget(m_widgetSigmaAGrayscale);
    layoutA->setAlignment(Qt::AlignLeft);
    QGroupBox *groupBoxA = new QGroupBox(tr("Sigma Absorption"));
    groupBoxA->setLayout(layoutA);

    m_g = new QDoubleSpinBox();
    m_g->setMaximum(0.999);
    m_g->setMinimum(-0.999);
    m_g->setSingleStep(0.01);
    m_g->setValue(material->g());
    m_g->setDecimals(4);
    QHBoxLayout *layoutG = new QHBoxLayout();
    layoutG->addWidget(new QLabel("Henyey Greenstein g: "));
    layoutG->addWidget(m_g);
    layoutG->setContentsMargins(0, 0, 0, 0);
    layoutG->setAlignment(Qt::AlignLeft);
    QWidget *widgetHG = new QWidget();
    widgetHG->setLayout(layoutG);

    m_type = new QComboBox();
    m_type->addItem("Grayscale");
    m_type->addItem("RGB");
    m_type->setCurrentIndex(1);
    connect(m_type, SIGNAL(currentIndexChanged(int)), this, SLOT(onTypeChanged(int)));
    m_widgetSigmaAGrayscale->hide();
    m_widgetSigmaSGrayscale->hide();
    m_widgetSigmaARGB->show();
    m_widgetSigmaSRGB->show();

    auto itr = materialTypeNames.find(MaterialType::MATERIAL_VOLUME);
    QVBoxLayout *layoutMain = new QVBoxLayout();
    layoutMain->addWidget(new QLabel(QString::fromStdString("Type: " + itr->second)));
    layoutMain->addWidget(m_type);
    layoutMain->addWidget(groupBoxS);
    layoutMain->addWidget(groupBoxA);
    layoutMain->addWidget(widgetHG);
    layoutMain->setSpacing(5);
    layoutMain->setAlignment(Qt::AlignTop);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
    setFixedHeight(HEIGHT);

    connect(m_sigmaS_R, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaS_R);
    connect(m_sigmaS_G, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaS_G);
    connect(m_sigmaS_B, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaS_B);
    connect(m_sigmaS_RGB, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaS_RGB);
    connect(m_sigmaA_R, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaA_R);
    connect(m_sigmaA_G, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaA_G);
    connect(m_sigmaA_B, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaA_B);
    connect(m_sigmaA_RGB, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onSigmaA_RGB);
    connect(m_g, &QDoubleSpinBox::valueChanged, this, &WidgetMaterialVolume::onGChanged);
}

QDoubleSpinBox *WidgetMaterialVolume::createSpinBox(float startingValue)
{
    QDoubleSpinBox *dsb = new QDoubleSpinBox();
    dsb->setMaximum(200);
    dsb->setMinimum(0);
    dsb->setSingleStep(0.1);
    dsb->setDecimals(4);
    dsb->setValue(startingValue);
    return dsb;
}

void WidgetMaterialVolume::onTypeChanged(int)
{
    if (m_type->currentIndex() == 0) {
        /* Changing to grayscale */
        m_widgetSigmaARGB->hide();
        m_widgetSigmaSRGB->hide();
        m_widgetSigmaAGrayscale->show();
        m_widgetSigmaSGrayscale->show();

        float sigma_a = glm::max(glm::max(m_sigmaA_R->value(), m_sigmaA_G->value()), m_sigmaA_B->value());
        float sigma_s = glm::max(glm::max(m_sigmaS_R->value(), m_sigmaS_G->value()), m_sigmaS_B->value());
        m_sigmaA_RGB->setValue(sigma_a);
        m_sigmaS_RGB->setValue(sigma_s);
        m_material->sigmaS() = glm::vec4(m_sigmaS_RGB->value());
        m_material->sigmaA() = glm::vec4(m_sigmaA_RGB->value());

    } else if (m_type->currentIndex() == 1) {
        /* Changing to RGB */
        m_widgetSigmaAGrayscale->hide();
        m_widgetSigmaSGrayscale->hide();
        m_widgetSigmaARGB->show();
        m_widgetSigmaSRGB->show();

        float sigma_a = m_sigmaA_RGB->value();
        float sigma_s = m_sigmaS_RGB->value();
        m_sigmaA_R->setValue(sigma_a);
        m_sigmaA_G->setValue(sigma_a);
        m_sigmaA_B->setValue(sigma_a);
        m_sigmaS_R->setValue(sigma_s);
        m_sigmaS_G->setValue(sigma_s);
        m_sigmaS_B->setValue(sigma_s);
        m_material->sigmaS() = glm::vec4(sigma_s);
        m_material->sigmaA() = glm::vec4(sigma_a);
    }
}

void WidgetMaterialVolume::onSigmaS_R(double)
{
    m_material->sigmaS().r = m_sigmaS_R->value();
}

void WidgetMaterialVolume::onSigmaS_G(double)
{
    m_material->sigmaS().g = m_sigmaS_G->value();
}

void WidgetMaterialVolume::onSigmaS_B(double)
{
    m_material->sigmaS().b = m_sigmaS_B->value();
}

void WidgetMaterialVolume::onSigmaS_RGB(double val)
{
    m_material->sigmaS() = glm::vec4(val);
}

void WidgetMaterialVolume::onSigmaA_R(double)
{
    m_material->sigmaA().r = m_sigmaA_R->value();
}

void WidgetMaterialVolume::onSigmaA_G(double)
{
    m_material->sigmaA().g = m_sigmaA_G->value();
}

void WidgetMaterialVolume::onSigmaA_B(double)
{
    m_material->sigmaA().b = m_sigmaA_B->value();
}

void WidgetMaterialVolume::onSigmaA_RGB(double val)
{
    m_material->sigmaA() = glm::vec4(val);
}

void WidgetMaterialVolume::onGChanged(double)
{
    m_material->g() = m_g->value();
}
