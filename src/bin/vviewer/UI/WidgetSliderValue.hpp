#ifndef __WidgetSliderValue_hpp__
#define __WidgetSliderValue_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qslider.h>

class WidgetSliderValue : public QWidget {
    Q_OBJECT
public:
    WidgetSliderValue(QWidget * parent, float min, float max, float value, float step, int scale = 100);

    void setValue(float value);
    float getValue();

private:
    int m_scale;
    QSlider * m_slider;
    QDoubleSpinBox * m_spinBox;

public slots:
    void onSliderChanged(int);
    void onSpinBoxChanged(double);

signals:
    void valueChanged(double);
};

#endif