#ifndef __WidgetMaterialLambert_hpp__
#define __WidgetMaterialLambert_hpp__

#include <memory>

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>
#include <qgroupbox.h>

#include "vengine/core/Materials.hpp"
#include "vengine/core/SceneObject.hpp"

/* A UI widget to represent a lambert material */
class WidgetMaterialLambert : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 525;

    WidgetMaterialLambert(QWidget *parent, vengine::MaterialLambert *material);

    QPushButton *m_colorAlbedo, *m_colorEmissive;
    QSlider *m_ao, *m_alpha;
    QDoubleSpinBox *m_emissive;
    QGroupBox *m_groupBoxAlpha;

    QComboBox *m_comboBoxAlbedo;
    QComboBox *m_comboBoxAO;
    QComboBox *m_comboBoxNormal;
    QComboBox *m_comboBoxEmissive;
    QComboBox *m_comboBoxAlpha;

    QDoubleSpinBox *m_uTiling;
    QDoubleSpinBox *m_vTiling;

    void updateAvailableTextures();

private:
    vengine::MaterialLambert *m_material = nullptr;

private Q_SLOTS:
    void onAlbedoButton();
    void onAlbedoTextureChanged(int);
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