#ifndef __WidgetMaterialPBR_hpp__
#define __WidgetMaterialPBR_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qgroupbox.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"

/* A UI widget to represent a PBR material */
class WidgetMaterialPBR : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 715;

    WidgetMaterialPBR(QWidget *parent, vengine::MaterialPBRStandard *material);

    QPushButton *m_colorAlbedo, *m_colorEmissive;
    QSlider *m_metallic, *m_roughness, *m_ao, *m_alpha;
    QDoubleSpinBox *m_emissive;
    QGroupBox *m_groupBoxAlpha;

    QComboBox *m_comboBoxAlbedo;
    QComboBox *m_comboBoxMetallic;
    QComboBox *m_comboBoxRoughness;
    QComboBox *m_comboBoxAO;
    QComboBox *m_comboBoxEmissive;
    QComboBox *m_comboBoxNormal;
    QComboBox *m_comboBoxAlpha;

    QDoubleSpinBox *m_uTiling;
    QDoubleSpinBox *m_vTiling;

    void updateAvailableTextures();

private:
    vengine::MaterialPBRStandard *m_material = nullptr;

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
    void onAlphaStateChanged(bool);
    void onAlphaChanged();
    void onAlphaTextureChanged(int);
    void onUTilingChanged(double);
    void onVTilingChanged(double);
};

#endif