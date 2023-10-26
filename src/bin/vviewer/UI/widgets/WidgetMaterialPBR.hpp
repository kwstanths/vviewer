#ifndef __WidgetMaterialPBR_hpp__
#define __WidgetMaterialPBR_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"

/* A UI widget to represent a PBR material */
class WidgetMaterialPBR : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 620;

    WidgetMaterialPBR(QWidget *parent, std::shared_ptr<vengine::MaterialPBRStandard> material);

    QPushButton *m_colorAlbedo, *m_colorEmissive;
    QSlider *m_metallic, *m_roughness, *m_ao;
    QDoubleSpinBox *m_emissive;

    QComboBox *m_comboBoxAlbedo;
    QComboBox *m_comboBoxMetallic;
    QComboBox *m_comboBoxRoughness;
    QComboBox *m_comboBoxAO;
    QComboBox *m_comboBoxEmissive;
    QComboBox *m_comboBoxNormal;

    QDoubleSpinBox *m_uTiling;
    QDoubleSpinBox *m_vTiling;

private:
    std::shared_ptr<vengine::MaterialPBRStandard> m_material = nullptr;

    void setColorButton(QPushButton *button, const glm::vec4 &color);

private Q_SLOTS:
    void onAlbedoButton();
    void onAlbedoTextureChanged(int);
    void onMetallicChanged();
    void onMetallicTextureChanged(int);
    void onRoughnessChanged();
    void onRoughnessTextureChanged(int);
    void onAOChanged();
    void onAOTextureChanged(int);
    void onEmissiveChanged(double);
    void onEmissiveButton();
    void onEmissiveTextureChanged(int);
    void onNormalTextureChanged(int);
    void onUTilingChanged(double);
    void onVTilingChanged(double);
};

#endif