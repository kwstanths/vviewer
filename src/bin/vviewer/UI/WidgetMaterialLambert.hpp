#ifndef __WidgetMaterialLambert_hpp__
#define __WidgetMaterialLambert_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a lambert material */
class WidgetMaterialLambert: public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialLambert(QWidget* parent, MaterialLambert* material);

    QPushButton* m_colorButton;
    QSlider* m_ao, * m_emissive;

    QComboBox* m_comboBoxAlbedo;
    QComboBox* m_comboBoxAO;
    QComboBox* m_comboBoxEmissive;
    QComboBox* m_comboBoxNormal;

private:
    MaterialLambert* m_material = nullptr;

    void setColorButtonColor();

private slots:
    void onColorButton();
    void onColorChanged(QColor color);
    void onColorTextureChanged(int);
    void onAOChanged();
    void onAOTextureChanged(int);
    void onEmissiveChanged();
    void onEmissiveTextureChanged(int);
    void onNormalTextureChanged(int);
};

#endif