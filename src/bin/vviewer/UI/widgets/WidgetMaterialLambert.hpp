#ifndef __WidgetMaterialLambert_hpp__
#define __WidgetMaterialLambert_hpp__

#include <memory>

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a lambert material */
class WidgetMaterialLambert : public QWidget
{
    Q_OBJECT
public:
    static const int HEIGHT = 430;

    WidgetMaterialLambert(QWidget *parent, std::shared_ptr<vengine::MaterialLambert> material);

    QPushButton *m_colorAlbedo, *m_colorEmissive;
    QSlider *m_ao;
    QDoubleSpinBox *m_emissive;

    QComboBox *m_comboBoxAlbedo;
    QComboBox *m_comboBoxAO;
    QComboBox *m_comboBoxNormal;
    QComboBox *m_comboBoxEmissive;

    QDoubleSpinBox *m_uTiling;
    QDoubleSpinBox *m_vTiling;

private:
    std::shared_ptr<vengine::MaterialLambert> m_material = nullptr;

    void setColorButton(QPushButton *button, const glm::vec4 &color);

private Q_SLOTS:
    void onAlbedoButton();
    void onAlbedoTextureChanged(int);
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