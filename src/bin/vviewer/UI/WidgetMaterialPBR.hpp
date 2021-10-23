#ifndef __WidgetMaterialPBR_hpp__
#define __WidgetMaterialPBR_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>

#include "core/Materials.hpp"

/* A UI widget to represent a transform */
class WidgetMaterialPBR : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialPBR(QWidget * parent, MaterialPBR * material);

    QPushButton * m_colorButton;
    QSlider *m_metallic, *m_roughness, *m_ao, *m_emissive;
private:
    MaterialPBR * m_material = nullptr;

    void setColorButtonColor();

private slots:
    void onColorButton();
    void onColorChanged(QColor color);
    void onMetallicChanged();
    void onRoughnessChanged();
    void onAOChanged();
    void onEmissiveChanged();

};

#endif