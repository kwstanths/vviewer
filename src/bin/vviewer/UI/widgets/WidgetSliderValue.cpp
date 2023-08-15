#include "WidgetSliderValue.hpp"

#include "qgroupbox.h"
#include "qlayout.h"

WidgetSliderValue::WidgetSliderValue(QWidget * parent, float min, float max, float value, float step, int scale) : QWidget(parent)
{
    m_scale = scale;

    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setMinimum(min * m_scale);
    m_slider->setMaximum(max * m_scale);
    m_slider->setValue(value * m_scale);
    connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));

    m_spinBox = new QDoubleSpinBox();
    m_spinBox->setMinimum(min);
    m_spinBox->setMaximum(max);
    m_spinBox->setValue(value);
    m_spinBox->setSingleStep(step);
    connect(m_spinBox, &QDoubleSpinBox::valueChanged, this, &WidgetSliderValue::onSpinBoxChanged);

    QHBoxLayout * layoutMain = new QHBoxLayout();
    layoutMain->addWidget(m_slider);
    layoutMain->addWidget(m_spinBox);
    layoutMain->setContentsMargins(0, 0, 0, 0);

    setLayout(layoutMain);
}

void WidgetSliderValue::setValue(float value)
{
    m_slider->setValue(value * m_scale);
    m_spinBox->setValue(value);
}

float WidgetSliderValue::getValue()
{
    return m_spinBox->value();
}

void WidgetSliderValue::onSliderChanged(int)
{
    double value = (double) m_slider->value() / m_scale;
    m_spinBox->setValue(value);
    Q_EMIT valueChanged(value);
}

void WidgetSliderValue::onSpinBoxChanged(double)
{
    m_slider->setValue(m_spinBox->value() * m_scale);
    Q_EMIT valueChanged(m_spinBox->value());
}
