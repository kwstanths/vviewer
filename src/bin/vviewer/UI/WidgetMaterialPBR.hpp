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
    WidgetMaterialPBR(QWidget * parent, SceneObject * sceneObject, MaterialPBR * material, QStringList availableMaterials);

    QComboBox * m_comboBoxAvailableMaterials;
    QPushButton * m_colorButton;
    QSlider *m_metallic, *m_roughness, *m_ao, *m_emissive;
    
private:
    MaterialPBR * m_material = nullptr;
    SceneObject * m_sceneObject = nullptr;

    void setColorButtonColor();

private slots:
    void onMaterialChanged(int);
    void onColorButton();
    void onColorChanged(QColor color);
    void onMetallicChanged();
    void onRoughnessChanged();
    void onAOChanged();
    void onEmissiveChanged();

};

#endif