#ifndef __WidgetMaterialPBR_hpp__
#define __WidgetMaterialPBR_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a PBR material */
class WidgetMaterialPBR : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialPBR(QWidget * parent, MaterialPBRStandard * material);

    QPushButton * m_colorButton;
    QSlider *m_metallic, *m_roughness, *m_ao;
    QDoubleSpinBox *m_emissive;

    QComboBox * m_comboBoxAlbedo;
    QComboBox * m_comboBoxMetallic;
    QComboBox * m_comboBoxRoughness;
    QComboBox * m_comboBoxAO;
    QComboBox * m_comboBoxNormal;
    
    QDoubleSpinBox * m_uTiling;
    QDoubleSpinBox * m_vTiling;
private:
    MaterialPBRStandard * m_material = nullptr;

    void setColorButtonColor();

private slots:
    void onColorButton();
    void onColorChanged(QColor color);
    void onColorTextureChanged(int);
    void onMetallicChanged();
    void onMetallicTextureChanged(int);
    void onRoughnessChanged();
    void onRoughnessTextureChanged(int);
    void onAOChanged();
    void onAOTextureChanged(int);
    void onEmissiveChanged(double);
    void onNormalTextureChanged(int);
    void onUTilingChanged(double);
    void onVTilingChanged(double);
};

#endif