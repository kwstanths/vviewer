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
class WidgetMaterialLambert: public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialLambert(QWidget* parent, std::shared_ptr<MaterialLambert> material);

    QPushButton* m_colorButton;
    QSlider* m_ao;
    QDoubleSpinBox* m_emissive;

    QComboBox* m_comboBoxAlbedo;
    QComboBox* m_comboBoxAO;
    QComboBox* m_comboBoxNormal;

private:
    std::shared_ptr<MaterialLambert> m_material = nullptr;

    void setColorButtonColor();

private Q_SLOTS:
    void onColorButton();
    void onColorChanged(QColor color);
    void onColorTextureChanged(int);
    void onAOChanged();
    void onAOTextureChanged(int);
    void onEmissiveChanged(double);
    void onNormalTextureChanged(int);
};

#endif