#ifndef __WidgetMaterialVolume_hpp__
#define __WidgetMaterialVolume_hpp__

#include <memory>

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qgroupbox.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"

#include "WidgetSliderValue.hpp"

/* A UI widget to represent a volume material */
class WidgetMaterialVolume : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 220;

    WidgetMaterialVolume(QWidget *parent, vengine::MaterialVolume *material);

    QComboBox *m_type;

    QDoubleSpinBox *m_sigmaS_R;
    QDoubleSpinBox *m_sigmaS_G;
    QDoubleSpinBox *m_sigmaS_B;
    QWidget *m_widgetSigmaSGrayscale;
    QDoubleSpinBox *m_sigmaS_RGB;
    QWidget *m_widgetSigmaSRGB;
    QDoubleSpinBox *m_sigmaA_R;
    QDoubleSpinBox *m_sigmaA_G;
    QDoubleSpinBox *m_sigmaA_B;
    QWidget *m_widgetSigmaAGrayscale;
    QDoubleSpinBox *m_sigmaA_RGB;
    QWidget *m_widgetSigmaARGB;
    QDoubleSpinBox *m_g;

private:
    vengine::MaterialVolume *m_material = nullptr;

    QDoubleSpinBox *createSpinBox(float startingValue);

private Q_SLOTS:
    void onTypeChanged(int);

    void onSigmaS_R(double);
    void onSigmaS_G(double);
    void onSigmaS_B(double);
    void onSigmaS_RGB(double);
    void onSigmaA_R(double);
    void onSigmaA_G(double);
    void onSigmaA_B(double);
    void onSigmaA_RGB(double);
    void onGChanged(double);
};

#endif