#ifndef __WidgetMaterial_hpp__
#define __WidgetMaterial_hpp__

#include <qwidget.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include "core/Materials.hpp"
#include "core/SceneObject.hpp"

/* A UI widget to represent a material */
class WidgetMaterial : public QWidget
{
    Q_OBJECT
public:
    WidgetMaterial(QWidget* parent, SceneObject* sceneObject);

    QComboBox* m_comboBoxAvailableMaterials;

private:
    SceneObject* m_sceneObject = nullptr;
    QWidget* m_materialWidget = nullptr;

    void createUI(QWidget* widgetMaterial);
    QWidget* createMaterialWidget(Material * material);

    QGroupBox* m_widgetGroupBox = nullptr;
    QVBoxLayout* m_layoutGroupBox = nullptr;
    QVBoxLayout* m_layoutMain = nullptr;
    QWidget* m_widgetMaterial = nullptr;

private slots:
    void onMaterialChanged(int);
};

#endif