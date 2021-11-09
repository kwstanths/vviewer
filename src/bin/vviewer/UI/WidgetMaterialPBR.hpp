#ifndef __WidgetMaterialPBR_hpp__
#define __WidgetMaterialPBR_hpp__

#include <qwidget.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qcombobox.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a transform */
class WidgetMaterialPBR : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterialPBR(QWidget * parent, SceneObject * sceneObject, MaterialPBR * material, 
        QStringList availableMaterials,
        QStringList availableTextures);

    QComboBox * m_comboBoxAvailableMaterials;
    QPushButton * m_colorButton;
    QSlider *m_metallic, *m_roughness, *m_ao, *m_emissive;

    QComboBox * m_comboBoxAlbedo;
    QComboBox * m_comboBoxMetallic;
    QComboBox * m_comboBoxRoughness;
    QComboBox * m_comboBoxAO;
    QComboBox * m_comboBoxEmissive;
    QComboBox * m_comboBoxNormal;
    
private:
    MaterialPBR * m_material = nullptr;
    SceneObject * m_sceneObject = nullptr;

    void setColorButtonColor();

private slots:
    void onMaterialChanged(int);
    void onColorButton();
    void onColorChanged(QColor color);
    void onColorTextureChanged(int);
    void onMetallicChanged();
    void onMetallicTextureChanged(int);
    void onRoughnessChanged();
    void onRoughnessTextureChanged(int);
    void onAOChanged();
    void onAOTextureChanged(int);
    void onEmissiveChanged();
    void onEmissiveTextureChanged(int);
    void onNormalTextureChanged(int);

};

#endif